// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

// Show users how to run the program
void printUsage()
{
  printf("Usage with optional options: ./lab0 --input=filename1 --output=filename2 --segfault --catch\n");
}

// Read from the input and write to the output
void readThenWrite(int input_file_descriptor, int output_file_descriptor)
{
  // Allocate space for the read, and then immediately write until the end of the file is reached
  char* charArray;
  charArray = (char*)malloc(sizeof(char));
  int NOT_END_OF_FILE = read(input_file_descriptor, charArray, 1);
  while(NOT_END_OF_FILE > 0)
  {
    write(output_file_descriptor, charArray, 1);
    NOT_END_OF_FILE = read(input_file_descriptor, charArray, 1);
  }

  // Always free memory after a malloc call
  free(charArray);
}

// Create a segmentation fault by trying to dereference a NULL pointer
void createSegmentationFault()
{
  char* empty = NULL;
  char a = *empty;
}

// Handle the signal call and print to stderr, using strerror to generate an error message
void catchSegmentationFault(int signal)
{
  if (signal == SIGSEGV)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(4);
  }
}

// Main
int main(int argc, char **argv)
{ 
  // Initialize options
  static struct option long_options[] =
  {
    {"input",    required_argument,   0,     'i'   },
    {"output",   required_argument,   0,     'o'   },
    {"segfault", no_argument,         0,     's'   },
    {"catch",    no_argument,         0,     'c'   },
    {0,          0,                   0,      0    }
  }; 

  // Parse options
  int opt = 0;
  int long_index = 0;
  char* input_file_name = NULL;
  char* output_file_name = NULL;
  int seg_fault_flag = 0;
  int catch_flag = 0;
  while ((opt = getopt_long(argc, argv, "", long_options, &long_index )) != -1)
  {
    switch (opt)
    {
      case 'i':
	      input_file_name = optarg;
        break;
      case 'o':
	      output_file_name = optarg;
        break;
      case 's':
	      seg_fault_flag = 1;
        break;
      case 'c':
	      catch_flag = 1;
        break;
      default:
	      printUsage(); 
        exit(1);
    }
  }

  // Only catch the segmentation fault if the catch_flag is toggled
  if(catch_flag == 1)
  {
    signal(SIGSEGV, catchSegmentationFault);
  }
  
  // Only create a segmentation fault if the seg_fault_flag is toggled          
  if(seg_fault_flag == 1)
  {
    createSegmentationFault();
  }
  
  // If the input file name was read successfully from optarg
  if(input_file_name)
  {
    // Attempt to open the input file
    int input_file_descriptor = open(input_file_name, O_RDONLY);

    // If the input file cannot be open, write to stderr and exit
    if(input_file_descriptor < 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(2);
    }

    // If the input file was open successfully, apply redirection and store reference to file at 0
    else
    {
      close(0);
      dup(input_file_descriptor);
      close(input_file_descriptor);
    }
  }

  // If the output file name was read successfully from optarg
  if(output_file_name)
  {
    // Attempt to create a new file with the specified file name 
    int output_file_descriptor = creat(output_file_name, S_IRWXU);

    // If the file could not be created, write to stderr and exit
    if(output_file_descriptor < 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(3);
    }

    // If the file was created successfully, apply redirection and store reference to file at 1
    else
    {
      close(1);
      dup(output_file_descriptor);
      close(output_file_descriptor);
    }
  }
  
  // Read from file at 0 descriptor and write to file at 1 descriptor
  readThenWrite(0, 1);
  exit(0);
}
  
