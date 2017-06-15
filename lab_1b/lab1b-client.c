// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //

// CLIENT //  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <getopt.h>
#include <poll.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <mcrypt.h>
#include <netinet/in.h>
#include <netdb.h>

// GLOBAL VARIABLES //

// Size for the maximum size for buffer
int MAX_ARRAY_SIZE = 128;

// Option flags: 1 means option is active; 0 otherwise
int portFlag = 0;
int logFlag = 0;
int encryptFlag = 0;

// User arguments to long options
int portNumber = -1;
char* logFile = NULL;

// Socket file descriptor
int socketFD;
int logFileFD = -1;

// Terminal attributes
struct termios original_terminal;
struct termios new_terminal;

// Encryption Variables
int keyLength;
char* ENCRYPTION_ALGORITHM = "blowfish";
char* CIPHER_FEEDBACK_MODE = "cfb";
MCRYPT encryptFD;
MCRYPT decryptFD;

// HELPER FUNCTION DECLARATIONS //

void resetTerminalAttributesAndDeinitModules();
void setTerminalAttributes();
void processInput();
void printUsage();
void initializeSocketServer();
char* getFileContents(char* file);

// ENCRYPTION / DECRYPTION DECLARATIONS //

void initializeEncryptionAndDecryption(char* key, int keyLength);
void deinitializeEncryptionAndDecryption();
void encryptCharArray(char* charArray, int encryptionLength);
void decryptCharArray(char* charArray, int decryptionLength);

// HELPER FUNCTION IMPLEMENTATIONS //

// Reset the terminal attributes
void resetTerminalAttributesAndDeinitModules()
{
  // Set the attributes back to the original, saved attributes
  int setAttrStatus = tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
  if(setAttrStatus < 0)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // If the encryption mode was on, deinitialize the encryption after restoring terminal attributes
  if(encryptFlag == 1)
  {
    deinitializeEncryptionAndDecryption();
  }
}

// Set the terminal attributes
void setTerminalAttributes()
{
  // Verify that STDIN is a valid terminal
  int isAttyStatus = isatty(STDIN_FILENO);
  if(isAttyStatus == 0)
  {
    fprintf (stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Save the current terminal attributes into the variable original_terminal
  int getAttrStatus = tcgetattr(STDIN_FILENO, &original_terminal);
  if(getAttrStatus < 0)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Get new terminal attributes
  getAttrStatus = tcgetattr(STDIN_FILENO, &new_terminal);
  if (getAttrStatus < 0)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Make settings non-canonical and non-echo
  new_terminal.c_iflag = ISTRIP;
  new_terminal.c_oflag = 0;
  new_terminal.c_lflag = 0;
  new_terminal.c_lflag &= ~(ECHO|ICANON);
  new_terminal.c_cc[VMIN] = 1;
  new_terminal.c_cc[VTIME] = 0;

  // Check if the terminal settings were correctly applied
  int setAttrStatus = tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
  if(setAttrStatus < 0)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
}

// Process the input from the input file descriptor and output it correctly depending on pipe ID
void processInput(int fd_read, int fd_write, int readingFromKeyboard)
{ 
  char charArray[MAX_ARRAY_SIZE];
  int currentByte;
  int writeStatus;
  int endProgram = 0;
  ssize_t byteCount = read(fd_read, charArray, 1);

  // If an EOF is detected from reading from the socket
  if(byteCount == 0)
  {
    // Reset the terminal attributes
    resetTerminalAttributesAndDeinitModules();

    // Exit with RC = 0
    exit(0);
  }
  
  // For each byte read
  for(currentByte = 0; currentByte < byteCount; currentByte++)
  {
    // If reading from keyboard
    if(readingFromKeyboard == 1)
    {
      // If a carriage return is encountered, echo a new line
      if(charArray[currentByte] == '\r')
      {
        writeStatus = write(STDOUT_FILENO, "\n", 1);
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }
      }

      // Write to screen
      writeStatus = write(STDOUT_FILENO, &charArray[currentByte], 1);
      if(writeStatus < 0)
      {
        resetTerminalAttributesAndDeinitModules();
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
      }

      // If encrypt flag is high, encrypt
      if(encryptFlag == 1)
      {
        encryptCharArray(&charArray[currentByte], 1);
      }

      // Write to log file
      if(logFlag == 1)
      {
        // Base message
        char sentMessage[14] = "SENT # bytes: "; 
        sentMessage[5] = byteCount + '0';

        // Write base message with replaced number of bytes
        writeStatus = write(logFileFD, sentMessage, 14);
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }

        // write post-encrypted character
        writeStatus = write(logFileFD, &charArray[currentByte], 1);
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }

        // Write a new line for the next log line
        writeStatus = write(logFileFD, "\n", 1);        
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }
      } 

      // Send to socket
      writeStatus = write(fd_write, &charArray[currentByte], 1);
      if(writeStatus < 0)
      {
        resetTerminalAttributesAndDeinitModules();
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
      }
    }

    // If reading from socket
    else 
    {
      // Write to log file
      if(logFlag == 1)
      {
        // Base message
        char receivedMessage[18] = "RECEIVED # bytes: "; 
        receivedMessage[9] = byteCount + '0';

        // Write base message with replaced number of bytes
        writeStatus = write(logFileFD, receivedMessage, 18);
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }

        // Write the pre-decrypted character
        writeStatus = write(logFileFD, &charArray[currentByte], 1);
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }
        
        // Write a new line for the next log line
        writeStatus = write(logFileFD, "\n", 1);        
        if(writeStatus < 0)
        {
          resetTerminalAttributesAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }
      } 

      // Decrypt if encryptFlag is high
      if(encryptFlag == 1)
      {
        decryptCharArray(&charArray[currentByte], 1);
      }

      // Write to screen
      writeStatus = write(STDOUT_FILENO, &charArray[currentByte], 1);
      if(writeStatus < 0)
      {
        resetTerminalAttributesAndDeinitModules();
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
      }
    }
  }
}

// Print usage in case user tries to use an invalid option
void printUsage()
{
  printf("Correct usage: ./lab1b-client --port=#### --log=filename --encrypt=my.key\n");
}

// Initialize a server at the given port on localhost
void initializeSocketServer()
{
  struct hostent* socketServer;
  struct sockaddr_in serverAddress;

  // Save file descriptor for socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if(socketFD < 0)
  {
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Use localhost for server
  socketServer = gethostbyname("localhost");
  if(socketServer == NULL)
  {
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  //Initialize the server address to zero and then correctly assign it
  memset((char*) &serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  memcpy((char *) &serverAddress.sin_addr.s_addr, (char*) socketServer->h_addr, socketServer->h_length);
  serverAddress.sin_port = htons(portNumber);

  //Connect to our established server using the socket
  int connectStatus = connect(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  if(connectStatus < 0) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
}

// Grab contents of file to obtain encryption key
char* getFileContents(char* file)
{
  // Open the encryption file
  struct stat fileStat;
  int fileFD = open(file, O_RDONLY);
  int fstatStatus = fstat(fileFD, &fileStat);
  if(fstatStatus < 0) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Grab the file contents
  int fileStatSize = fileStat.st_size;
  char* fileContents = (char*) malloc(fileStatSize * sizeof(char));

  // Read from the file
  int readStatus = read(fileFD, fileContents, fileStatSize);
  if(readStatus < 0) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  // Set the length of the key and return the file contents
  keyLength = fileStatSize;
  return fileContents;
}

// ENCRYPTION / DECRYPTION IMPLEMENTATIONS //

// Initialize the encryption and decryption modules
void initializeEncryptionAndDecryption(char* key, int keyLength)
{
  int genericInitStatus = -1;
  encryptFD = mcrypt_module_open(ENCRYPTION_ALGORITHM, NULL, CIPHER_FEEDBACK_MODE, NULL);
  if(encryptFD == MCRYPT_FAILED) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  genericInitStatus = mcrypt_generic_init(encryptFD, key, keyLength, NULL);
  if(genericInitStatus < 0) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  decryptFD = mcrypt_module_open(ENCRYPTION_ALGORITHM, NULL, CIPHER_FEEDBACK_MODE, NULL);
  if(decryptFD == MCRYPT_FAILED) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  genericInitStatus = mcrypt_generic_init(decryptFD, key, keyLength, NULL);
  if(genericInitStatus < 0)
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }
}

// Deinitialize the encryption and decryption modules
void deinitializeEncryptionAndDecryption()
{
  mcrypt_generic_deinit(encryptFD);
  mcrypt_module_close(encryptFD);
  mcrypt_generic_deinit(decryptFD);
  mcrypt_module_close(decryptFD);
}

// Runs the blowfish ofb encryption algorithm on specified buffer for given size
void encryptCharArray(char* charArray, int encryptionLength)
{
  int encryptionStatus = mcrypt_generic(encryptFD, charArray, encryptionLength);
  if(encryptionStatus != 0) 
  { 
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }
}

// Runs the corresponding decryption algorithm on the specified buffer for the size
void decryptCharArray(char* charArray, int decryptionLength)
{
  int decryptionStatus = mdecrypt_generic(decryptFD, charArray, decryptionLength);
  if(decryptionStatus != 0)
  {
    resetTerminalAttributesAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }
}

// MAIN //

int main(int argc, char **argv)
{ 
  int getOptStatus = 0;

  // Struct with the available long options
  static struct option long_options[] =
  {
    { "port",    required_argument, 0, 'p' },
    { "log",     required_argument, 0, 'l' },
    { "encrypt", required_argument, 0, 'e' },
    { 0,         0,                 0,  0  }
  };

  // Loop through options
  while((getOptStatus = getopt_long(argc, argv, "", long_options, NULL)) != -1)
  {
    switch(getOptStatus)
    {
      case 'p':
        portFlag = 1;
        portNumber = atoi(optarg);
        break;
      case 'l':
        logFile = optarg;
        break;
      case 'e':
        encryptFlag = 1;
        char* key = getFileContents(optarg);
        initializeEncryptionAndDecryption(key, keyLength);
        break;
      default:
	      // Print correct usage when finding an unrecognized argument
	      printUsage();
	      exit(1);
        break;
    }
  }

  // If the log file name was read successfully from optarg
  if(logFile != NULL)
  {
    // Attempt to create a new file with the specified file name 
    logFileFD = creat(logFile, 0666);

    // If the file could not be created, write to stderr and exit
    if(logFileFD < 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
    
    // Only set the logFlag if the file was successfully created
    logFlag = 1;
  }

  // Set the terminal attributes with custom options (non-canonical)
  setTerminalAttributes();

  // If we are in the port option mode
  if(portFlag == 1)
  {
    // Initialize socket file descriptor
    initializeSocketServer();

    // Implement poll options
    struct pollfd fds[2];
    memset(&fds, 0, sizeof(fds));
    fds[0].fd = STDIN_FILENO;
    fds[1].fd = socketFD;
    fds[0].events = POLLIN | POLLERR | POLLHUP;
    fds[1].events = POLLIN | POLLERR | POLLHUP;

    // Continually poll
    while(1)
    {
      // Grab the result of the poll call
      int pollStatus = poll(fds, 2, 0);

      // If the poll call was successful
      if(pollStatus > 0)
      {
        // If the keyboard has input that is ready to be read
        if(fds[0].revents & POLLIN)
        { 
          processInput(STDIN_FILENO, socketFD, 1);
        }

        // If the server processed the bash and is ready to be read
        if(fds[1].revents & POLLIN)
        {
          processInput(socketFD, STDOUT_FILENO, 0);
        }

        // If one of the error flags becomes high, just exit(0) because we're done reading. Not system calls, so return 0
        if(fds[0].revents & POLLERR || fds[0].revents & POLLHUP || fds[1].revents & POLLERR || fds[1].revents & POLLHUP)
        { 
          resetTerminalAttributesAndDeinitModules();
          exit(0);
        }
      }
    }
  }
  exit(0);
}
