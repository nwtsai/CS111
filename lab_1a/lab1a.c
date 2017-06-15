// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //  

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <poll.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

// GLOBAL VARIABLES //

// Size for the maximum size for buffer
int MAX_ARRAY_SIZE = 128;

// 1 means shell option is active; 0 otherwise
int shellFlag;

// Two unidirectional pipes
int toShellPipe[2];
int fromShellPipe[2];
pid_t shell_pid;

// Terminal attributes
struct termios original_terminal;
struct termios new_terminal;

// FUNCTION DECLARATIONS //

void handler(int signal);
void resetTerminalAttributes();
void setTerminalAttributes();
void createPipes();
void rewirePipes();
void closePipes();
void processInput(int fd_read, int fd_write, char ID);
void executeBash();
void printUsage();
void printShellExitStatus();

// FUNCTION IMPLEMENTATIONS //

// Handler that responds when ^C is typed or a SIGPIPE happens
void handler(int signal)
{
  // SIGPIPE
  if(signal == SIGPIPE)
  {
    exit(0);
  }

  // SIGINT
  if(shellFlag && signal == SIGINT)
  {
    int killStatus = kill(shell_pid, SIGINT);
    if(killStatus < 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }
}

// Reset the terminal attributes
void resetTerminalAttributes()
{
  // Set the attributes back to the original, saved attributes
  int setAttrStatus = tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
  if(setAttrStatus < 0)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
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

// Create the pipes
void createPipes()
{
  int toShellPipeStatus = pipe(toShellPipe);
  int fromShellPipeStatus = pipe(fromShellPipe);

  // Check if pipes could not be created
  if(toShellPipeStatus < 0 || fromShellPipeStatus < 0)
  {
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
  close(toShellPipe[0]);
  close(fromShellPipe[1]);
}

// Close the pipes
void closePipes()
{
  close(toShellPipe[0]);
  close(toShellPipe[1]);
  close(fromShellPipe[0]);
  close(fromShellPipe[1]);
}

// Process the input from the input file descriptor and output it correctly depending on pipe ID
void processInput(int fd_read, int fd_write, char ID)
{ 
  char charArray[MAX_ARRAY_SIZE];
  int currentByte;
  int writeStatus;
  ssize_t byteCount = read(fd_read, charArray, 1);

  // If the read was not successful, exit with return code 1 and report error
  if(!byteCount)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
  
  // For each byte read:
  for(currentByte = 0; currentByte < byteCount; currentByte++)
  {
    // When ^C is read, call handler to kill the shell process and exit with return code 0                                                                                                                                                                                     
    if(charArray[currentByte] == '\003')
    { 
      // If shell flag is on
      if(shellFlag == 1)
      {
	      handler(SIGINT);
      }

      // If in non-canonical mode, just exit
      else
      {
	      resetTerminalAttributes();
	      exit(0);
      }
    }
    
    // When ^D is read, reset the terminal attribute to the original, saved attributes
    else if(charArray[currentByte] == '\004')     
    {
      // Always reset terminal attributes when ^D is seen
      resetTerminalAttributes();
      
      // If shell flag is on
      if(shellFlag == 1)
      {
	      closePipes();
	      printShellExitStatus();
      }

      // Exit whenever ^D is seen
      exit(0);
    }     

    // If we encounter either \r or \n, just replace the character with the two characters \r\n 
    else if (charArray[currentByte] == '\r' || charArray[currentByte] == '\n') 
    {
      char cr_lf[2] = "\r\n";
      writeStatus = write(fd_write, &cr_lf, 2);
      if(writeStatus < 0)
      {
	      fprintf(stderr, "%s\n", strerror(errno));
	      exit(1);
      }

      // Write only \n to shell if pipe ID is 's' 
      if(ID == 's')
      {
        writeStatus = write(toShellPipe[1], &cr_lf[1], 1);
	      if(writeStatus < 0)
	      {
	        fprintf(stderr, "%s\n", strerror(errno));
	        exit(1);
	      }
      }
    }

    // If we encounter any other, non-special character
    else 
    {
      // Write to charArray
      writeStatus = write(fd_write, &charArray[currentByte], 1);
      if(writeStatus < 0)
      {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
      }
      
      // If pipe ID is 's', also write from the toShellPipe's write side to the charArray
      if(ID == 's')
      {
        writeStatus = write(toShellPipe[1], &charArray[currentByte], 1);
	      if(writeStatus < 0)
	      {
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
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
}

// Print usage in case user tries to use an invalid option
void printUsage()
{
  printf("Correct usage: ./lab1a --shell\n");
}

// Print the exit status of the shell
void printShellExitStatus()
{
  // If in shell mode, report status of exit
  if(shellFlag)
  {
    int shellStatus = 0;
    waitpid(shell_pid, &shellStatus, 0);

    // Grab different bits from shellStatus
    int shell_exit_signal = 0x007f & shellStatus;
    int shell_exit_status = 0xff00 & shellStatus;

    // Pring SIGNAL and STATUS #s
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", shell_exit_signal, shell_exit_status);
  }
}

// MAIN //

int main (int argc, char **argv)
{ 
  // Struct with the available long options
  static struct option long_options[] =
  {
    { "shell", no_argument, 0, 's' },
    { 0,       0,           0,  0  }
  };

  int getOptStatus = 0;
  shellFlag = 0;
  while((getOptStatus = getopt_long(argc, argv, "", long_options, NULL)) != -1)
  {
    switch(getOptStatus)
    {
      case 's':
	      // Set the signal handlers and toggle the shellFlag
        signal(SIGINT, handler);
        signal(SIGPIPE, handler);
	      shellFlag = 1;
        break;
      default:
	      // Print correct usage when finding an unrecognized argument
	      printUsage();
	      exit(1);
        break;
    }
  }

  // Set the terminal attributes with custom options
  setTerminalAttributes();

  // If we are not in the shell option mode
  if(shellFlag == 0)
  {
    // Continually process input from STDIN to STDOUT (essentially printing keyboard presses))
    while(1)
    {
      processInput(STDIN_FILENO,STDOUT_FILENO, 0);
    }
  }
  
  // If we are in the --shell option mode
  else
  {
    // Create pipes
    createPipes();

    // Attempt to fork
    shell_pid = fork();

    // If parent process comes from valid fork() call
    if(shell_pid > 0) 
    {
      // Close half of the pipe ends to avoid hang time
      close(toShellPipe[0]);
      close(fromShellPipe[1]);

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
	        // If the STDIN has input that is ready to be read
          if(fds[0].revents & POLLIN)
	        { 
            processInput(STDIN_FILENO, STDOUT_FILENO, 's');
          }

	        // If the fromShellPipe's read end is ready to be read
          if(fds[1].revents & POLLIN)
	        {
            processInput(fromShellPipe[0], STDOUT_FILENO, 'p');
          }

	        // If one of the error flags becomes high, just exit(0) because we're done reading. Not system calls, so return 0
          if(fds[0].revents & POLLERR || fds[0].revents & POLLHUP || fds[1].revents & POLLERR || fds[1].revents & POLLHUP)
          {
            resetTerminalAttributes();                                                                                                                                                                                                                                         
	          printShellExitStatus();
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
