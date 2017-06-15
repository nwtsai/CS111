// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //  

// SERVER //

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

// GLOBAL VARIABLES //

// Size for the maximum size for buffer
int MAX_ARRAY_SIZE = 128;

// 1 means shell option is active; 0 otherwise
int portFlag = 0;
int encryptFlag = 0;

// User arguments to long options
int portNumber = -1;

// Socket variables
int socketFD;
int serverSocketFD;

// Save the file descriptors
int SAVED_STDOUT;
int SAVED_STDIN;
int SAVED_STDERR;

// Two unidirectional pipes
int toShellPipe[2];
int fromShellPipe[2];
pid_t shell_pid;

// Encryption Variables
int keyLength;
char* ENCRYPTION_ALGORITHM = "blowfish";
char* CIPHER_FEEDBACK_MODE = "cfb";
MCRYPT encryptFD;
MCRYPT decryptFD;

// HELPER FUNCTION DECLARATIONS //

void handler(int signal);
void createPipes();
void rewirePipes();
void closePipes();
void processInput(int fd_read, int fd_write, int readingFromClient);
void executeBash();
void printUsage();
void printShellExitStatus();
void establishSocketConnection();
char* getFileContents(char* file);
void restoreFileDescriptorsAndDeinitModules();

// ENCRYPTION / DECRYPTION DECLARATIONS //

void initializeEncryptionAndDecryption(char* key, int keyLength);
void deinitializeEncryptionAndDecryption();
void encryptCharArray(char* charArray, int encryptionLength);
void decryptCharArray(char* charArray, int decryptionLength);

// FUNCTION IMPLEMENTATIONS //

// Handler that responds when ^C is typed or a SIGPIPE happens
void handler(int signal)
{
  // SIGPIPE
  if(signal == SIGPIPE)
  {
    restoreFileDescriptorsAndDeinitModules();
    printShellExitStatus();
    exit(0);
  }

  // SIGINT
  if(signal == SIGINT)
  {
    int killStatus = kill(shell_pid, SIGINT);
    if(killStatus < 0)
    {
      restoreFileDescriptorsAndDeinitModules();
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }
}

// Create the pipes
void createPipes()
{
  int toShellPipeStatus = pipe(toShellPipe);
  int fromShellPipeStatus = pipe(fromShellPipe);

  // Check if pipes could not be created
  if(toShellPipeStatus < 0 || fromShellPipeStatus < 0)
  {
    restoreFileDescriptorsAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
}

// Rewire pipes
void rewirePipes()
{
  close(toShellPipe[1]);
  close(fromShellPipe[0]);
  dup2(toShellPipe[0], STDIN_FILENO);
  dup2(fromShellPipe[1], STDOUT_FILENO);
  dup2(fromShellPipe[1], STDERR_FILENO);
  close(toShellPipe[0]);
  close(fromShellPipe[1]);
}

// Close the pipes
void closePipes()
{
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  close(toShellPipe[0]);
  close(toShellPipe[1]);
  close(fromShellPipe[0]);
  close(fromShellPipe[1]);
  close(socketFD);
}

// Process the input from the input file descriptor and output it correctly depending on pipe ID
void processInput(int fd_read, int fd_write, int readingFromClient)
{ 
  char charArray[MAX_ARRAY_SIZE];
  int currentByte;
  int writeStatus;
  ssize_t byteCount = read(fd_read, charArray, 1);

  // If server encounters an EOF
  if(byteCount == 0)
  {
    // Restore FDs and modules
    restoreFileDescriptorsAndDeinitModules();

    // SEND SIGTERM TO SHELL
    int killStatus = kill(shell_pid, SIGTERM);
    if(killStatus < 0)
    {
      restoreFileDescriptorsAndDeinitModules();
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }

    // Harvest shell exit status and exit
    printShellExitStatus();
    exit(0);
  }
  
  // For each byte read
  for(currentByte = 0; currentByte < byteCount; currentByte++)
  {
    // If encryptFlag is high and we are reading from client
    if(encryptFlag == 1 && readingFromClient == 1)
    {
      decryptCharArray(&charArray[currentByte], 1);
    }

    // When ^C is read, call handler to kill the shell process                                                                                                                                                                                     
    if(charArray[currentByte] == '\003')
    { 
      handler(SIGINT);
    }
    
    // When ^D is read, close write pipe to shell to trigger a POLLHUP
    else if(charArray[currentByte] == '\004')     
    {
      close(toShellPipe[1]);
    }     

    // If we encounter either \r or \n, just replace the character with the two characters \r\n 
    else if (charArray[currentByte] == '\r' || charArray[currentByte] == '\n') 
    {
      char cr_lf[2] = "\r\n";

      // If we are writing to the client
      if(readingFromClient == 0)
      {
        // Encrypt buffer before writing to client
        if(encryptFlag == 1)
        {
          encryptCharArray(cr_lf, 2);
        }

        writeStatus = write(fd_write, &cr_lf, 2);
        if(writeStatus < 0)
        {
          restoreFileDescriptorsAndDeinitModules();
  	      fprintf(stderr, "%s\n", strerror(errno));
  	      exit(1);
        }
      }

      // Write only \n to shell if reading from client
      if(readingFromClient == 1)
      {
        writeStatus = write(toShellPipe[1], &cr_lf[1], 1);
	      if(writeStatus < 0)
	      {
          restoreFileDescriptorsAndDeinitModules();
	        fprintf(stderr, "%s\n", strerror(errno));
	        exit(1);
	      }
      }
    }

    // If we encounter any other, non-special character
    else 
    {
      // If we are writing to the client
      if(readingFromClient == 0)
      {
        // Encrypt buffer before writing to client
        if(encryptFlag == 1)
        {
          encryptCharArray(&charArray[currentByte], 1);
        }

        // Write to charArray
        writeStatus = write(fd_write, &charArray[currentByte], 1);
        if(writeStatus < 0)
        {
          restoreFileDescriptorsAndDeinitModules();
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }
      }
      
      // If reading from the client
      if(readingFromClient == 1)
      {
        writeStatus = write(toShellPipe[1], &charArray[currentByte], 1);
	      if(writeStatus < 0)
	      {
          restoreFileDescriptorsAndDeinitModules();
	        fprintf(stderr, "%s\n", strerror(errno));
	        exit(1);
	      }
      }
    }
  }
}

// Execute bash
void executeBash()
{
  // First rewire pipes
  rewirePipes();

  // Exec to load the bash program and essentially leave this code base
  int execStatus = execvp("/bin/bash", NULL);
  if(execStatus < 0)
  {
    restoreFileDescriptorsAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
}

// Print usage in case user tries to use an invalid option
void printUsage()
{
  printf("Correct usage: ./lab1b-server --port=#### --encrypt=my.key\n");
}

// Print the exit status of the shell
void printShellExitStatus()
{
  // If in shell mode, report status of exit
  int shellStatus = 0;
  waitpid(shell_pid, &shellStatus, 0);

  // Grab different bits from shellStatus
  int shell_exit_signal = 0x007f & shellStatus;
  int shell_exit_status = 0xff00 & shellStatus;

  // Pring SIGNAL and STATUS #s
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", shell_exit_signal, shell_exit_status);
}

// Establish a connection with the client
void establishSocketConnection()
{
  int clientLength;
  struct sockaddr_in serverAddress;
  struct sockaddr_in clientAddress;

  // Save the socket file descriptor
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if(socketFD < 0) 
  {
    if(encryptFlag == 1)
    {
      deinitializeEncryptionAndDecryption();
    }
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Set address properties
  memset((char*) &serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(portNumber);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Bind server address
  int bindStatus = bind(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  if(bindStatus < 0)
  {
    if(encryptFlag == 1)
    {
      deinitializeEncryptionAndDecryption();
    }
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Listen for a network response to the socket
  listen(socketFD, 5);
  clientLength = sizeof(clientAddress);
  serverSocketFD = accept(socketFD, (struct sockaddr *) &clientAddress, &clientLength);
  if(serverSocketFD < 0)
  {
    if(encryptFlag == 1)
    {
      deinitializeEncryptionAndDecryption();
    }
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
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  // Set the length of the key and return the file contents
  keyLength = fileStatSize;
  return fileContents;
}

// Restore file descriptors so that all fprintf statements actually show up in terminal
void restoreFileDescriptorsAndDeinitModules()
{
  dup2(SAVED_STDOUT, STDOUT_FILENO);
  dup2(SAVED_STDIN, STDIN_FILENO);
  dup2(SAVED_STDERR, STDERR_FILENO);
  close(SAVED_STDOUT);
  close(SAVED_STDIN);
  close(SAVED_STDERR);

  // Deinitialize the encryption and decryption modules
  if(encryptFlag == 1)
  {
    deinitializeEncryptionAndDecryption();
  }
}

// ENCRYPTION / DECRYPTION IMPLEMENTATIONS //

// Initialize the encryption and decryption modules
void initializeEncryptionAndDecryption(char* key, int keyLength)
{
  int genericInitStatus = -1;
  encryptFD = mcrypt_module_open(ENCRYPTION_ALGORITHM, NULL, CIPHER_FEEDBACK_MODE, NULL);
  if(encryptFD == MCRYPT_FAILED) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  genericInitStatus = mcrypt_generic_init(encryptFD, key, keyLength, NULL);
  if(genericInitStatus < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  decryptFD = mcrypt_module_open(ENCRYPTION_ALGORITHM, NULL, CIPHER_FEEDBACK_MODE, NULL);
  if(decryptFD == MCRYPT_FAILED) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }

  genericInitStatus = mcrypt_generic_init(decryptFD, key, keyLength, NULL);
  if(genericInitStatus < 0)
  { 
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
    restoreFileDescriptorsAndDeinitModules();
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
    restoreFileDescriptorsAndDeinitModules();
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1); 
  }
}

// MAIN //

int main (int argc, char **argv)
{ 
  // Save the file descriptors
  SAVED_STDOUT = dup(STDOUT_FILENO);
  SAVED_STDIN = dup(STDIN_FILENO);
  SAVED_STDERR = dup(STDERR_FILENO);

  // Struct with the available long options
  static struct option long_options[] =
  {
    { "port",    required_argument, 0, 'p' },
    { "encrypt", required_argument, 0, 'e' },
    { 0,         0,                 0,  0  }
  };

  int getOptStatus = 0;
  while((getOptStatus = getopt_long(argc, argv, "", long_options, NULL)) != -1)
  {
    switch(getOptStatus)
    {
      case 'p':
        // Set the signal handlers
        signal(SIGINT, handler);
        signal(SIGPIPE, handler);
        portFlag = 1;
        portNumber = atoi(optarg);
        break;
      case 'e':
        encryptFlag = 1;
        char* key = getFileContents(optarg);
        initializeEncryptionAndDecryption(key, keyLength);
        break;
      default:
	      // Print correct usage when finding an unrecognized argument
        if(encryptFlag == 1)
        {
          deinitializeEncryptionAndDecryption();
        }
	      printUsage();
	      exit(1);
    }
  }

  // If we are not in the shell option mode
  if(portFlag == 1)
  {
    // Establish a connection to port
    establishSocketConnection();

    // Redirect file descriptors for STDIN, STDOUT, and STDERR to socket
    dup2(serverSocketFD, STDIN_FILENO);
    dup2(serverSocketFD, STDOUT_FILENO);
    dup2(serverSocketFD, STDERR_FILENO);
    close(serverSocketFD);

    // Create pipes
    createPipes();

    // Attempt to fork
    shell_pid = fork();

    // If parent process comes from valid fork() call
    if(shell_pid > 0) 
    {
      // Implement poll options
      struct pollfd fds[2];
      memset(&fds, 0, sizeof(fds));
      fds[0].fd = STDIN_FILENO;
      fds[1].fd = fromShellPipe[0];
      fds[0].events = POLLIN | POLLERR | POLLHUP;
      fds[1].events = POLLIN | POLLERR | POLLHUP;

      // Continually check if input is ready; if input is ready from the two fds, read and write accordingly
      while(1)
      {
      	// Grab the result of the poll call
      	int pollStatus = poll(fds, 2, 0);

      	// If the poll call was successful
        if(pollStatus > 0)
	      {
	        // If the server has received client input that is ready to be read
          if(fds[0].revents & POLLIN)
	        { 
            processInput(STDIN_FILENO, STDOUT_FILENO, 1);
          }

	        // If the fromShellPipe's read end is ready to be read
          if(fds[1].revents & POLLIN)
	        {
            // Close half of the pipe ends to avoid hang time
            close(toShellPipe[0]);
            close(fromShellPipe[1]);
            processInput(fromShellPipe[0], STDOUT_FILENO, 0);
          }

	        // Here, the to shell's write side has been closed, so a POLLHUP gets triggered for fd[1]
          if(fds[0].revents & POLLERR || fds[0].revents & POLLHUP || fds[1].revents & POLLERR || fds[1].revents & POLLHUP)
          {
            // Make 0, 1, 2 correspond to STDIN, STDOUT, and STDERR again
            restoreFileDescriptorsAndDeinitModules();

            // Collect and report shell termination status
            printShellExitStatus();

            // Close all pipes, including the network socket to client, and exit with RC = 0
            closePipes();
            exit(0);
          }
	      }
      }
    }

    // Child goes down this path
    else if(shell_pid == 0)
    {
      // Load and exec /bin/bash
      executeBash();
    }

    // Error occurred during forking; print error message
    else
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  exit(0);
}
