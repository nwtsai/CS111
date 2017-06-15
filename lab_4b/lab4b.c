// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //

#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <ctype.h>
#include <pthread.h>
#include <mraa/aio.h>
#include <mraa/i2c.h>

// GLOBAL VARIABLES //

// B constant
static const int B_CONST = 4275;

// Hardware variables
mraa_aio_context temperature_sensor;
mraa_gpio_context hardware_button;
int hardware_button_pressed = 0;

// Variable remains true as long as SIGINT doesn't happen
static sig_atomic_t volatile should_run = true;

// Variables that read user input
int period = 1;
sig_atomic_t volatile scale = 'F';
int log_flag = 0;
FILE* log_file = NULL;
char* log_filename = NULL;

// Time variable
time_t last_read_time;

// Thread variable
pthread_t thread;

// Flag for generating reports
int should_generate_reports = 1;

// HELPER FUNCTION DECLARATIONS //

void handler();
void processUserInput();
void checkHardwareButton();
void readTemperature();
void* threadFunction();
void printUsage();
void closeAll();

// HELP FUNCTION IMPLEMENTATIONS //

// Set the flag to 0 when the handler detects a SIGINT (^C)
void handler()
{
  should_run = 0;
}

// Process the user input
void processUserInput()
{
  // String for the user input
  char user_input[16];

  // Scan for the user's input, and process it once a \n is detected
  scanf("%s", user_input);

  // If the log flag is enabled, print the user's command exactly as received to the log file
  if(log_flag == 1)
  {
    fprintf(log_file, "%s\n", user_input);
  }
  
  // Check if the user input matches any recognized commands
  if(strcmp(user_input, "OFF") == 0)
  {
    // Holds the string value of the current timestamp
  	char time_stamp[16];

  	// Find local time and convert it into a string
    time_t my_time = time(NULL);
    struct tm* time_struct = localtime(&my_time);
    strftime(time_stamp, sizeof(time_stamp), "%H:%M:%S", time_struct);

	  // If the log flag is enabled, print the SHUTDOWN command to the log file
    if(log_flag == 1)
    {
  	  fprintf(log_file, "%s SHUTDOWN\n", time_stamp);
    }

	  // Close all and exit
	  closeAll();
    exit(0);
  }
  else if(strcmp(user_input, "STOP") == 0)
  {
  	should_generate_reports = 0;
  }
  else if(strcmp(user_input, "START") == 0)
  {
  	should_generate_reports = 1;
  }
  else if(strcmp(user_input, "SCALE=F") == 0)
  {
  	scale = 'F';
  }
  else if(strcmp(user_input, "SCALE=C") == 0)
  {
  	scale = 'C';
  }
  else if(strlen(user_input) > 7)
  {
  	// Grab the first 7 letters to see if it matches PERIOD=
  	char command[8];
  	strncpy(command, user_input, 7);
  	command[7] = '\0';
  	if(strcmp(command, "PERIOD=") == 0)
  	{
  	  // Grab the parameter from the character after PERIOD= to the end of the user's input
  	  int parameter_length = strlen(user_input) - 7;
  	  char parameter[parameter_length + 1];
  	  strncpy(parameter, user_input + 7, parameter_length);
  	  parameter[parameter_length + 1] = '\0';

  	  // Check if the parameter passed by the user is a valid integer
  	  int i;
  	  for(i = 0; i < parameter_length; i++)
  	  {
  	  	if(!isdigit(parameter[i]) && parameter[i] != '\0')
  	  	{
  	  	  if(log_flag == 1)
  	  	  {
  	  	    // Error message
  	  	    fprintf(log_file, "Error: Period is not a valid integer\n");
  	  	  }
  	  	  closeAll();
  	  	  exit(1);
  	  	}
  	  }

  	  // At this point, parameter must contain only the digits 0 to 9
  	  period = atoi(parameter);
  	}
  	else
  	{
  	  if(log_flag == 1)
  	  {
  	    // Error message
  	    fprintf(log_file, "Error: unrecognized command: %s\n", user_input);
  	  }
  	  closeAll();
  	  exit(1);
  	}
  }
  else
  {
  	if(log_flag == 1)
  	{
  	  // Error message
  	  fprintf(log_file, "Error: unrecognized command: %s\n", user_input);
  	}
  	closeAll();
  	exit(1);
  }
}

// Check if the hardware button is pressed
void checkHardwareButton()
{
  // If reading the sensor returns 1, the button has been pressed
  if(mraa_gpio_read(hardware_button) == 1)
  {
  	// String for the current time
    char time_stamp[16];

    // Toggle the button flag
    hardware_button_pressed = 1;

    // Find local time and convert it into a string
    time_t my_time = time(NULL);
    struct tm* time_struct = localtime(&my_time);
    strftime(time_stamp, sizeof(time_stamp), "%H:%M:%S", time_struct);

    // If the log flag is enabled, print final SHUTDOWN message with timestamp
	  if(log_flag == 1)
	  {
      fprintf(log_file, "%s SHUTDOWN\n", time_stamp);
	  }

    // Close all and exit
    closeAll();
    exit(0);
  }
}

// Read the temperature from the sensor and print to respective destinations
void readTemperature()
{  
  // String for the current time
  char time_stamp[16];

  // Set the new last_read_time
  last_read_time = time(0);

  // Find local time and convert it into a string
  time_t my_time = time(NULL);
  struct tm* time_struct = localtime(&my_time);
  strftime(time_stamp, sizeof(time_stamp), "%H:%M:%S", time_struct);

  // Read current temperature from the sensor
  uint16_t current_temperature = mraa_aio_read(temperature_sensor);
  float R_CONST = 1023.0 / ((float) current_temperature) - 1.0;
  R_CONST = 100000.0 * R_CONST;

  // Use datasheet to convert to a valid temperature
  float temperature = 1.0 / (log(R_CONST / 100000.0) / B_CONST + 1 / 298.15) - 273.15;

  // Convert from C to F, otherwise let it remain in Celsius (result of datasheet is by default Celsius)
  if(scale == 'F')
  {
    temperature = 32 + temperature * 1.8;
  }

  // If the temperature is less than 10, we need to add an extra 0 to ensure that the temperature reading is the correct width
  if(temperature < 10)
  {
  	// Print to standard output
    fprintf(stdout, "%s 0%.1f\n", time_stamp, temperature);

    // Send the data to the log file if the log flag is high
    if(log_flag == 1)
    {
      fprintf(log_file, "%s 0%.1f\n", time_stamp, temperature);
    }
  }
  else
  {
  	// Print to standard output
    fprintf(stdout, "%s %.1f\n", time_stamp, temperature);

    // Send the data to the log file if the log flag is high
    if(log_flag == 1)
    {
      fprintf(log_file, "%s %.1f\n", time_stamp, temperature);
    }
  }
}

// Thread checks hardware button and reads temperature under the specified conditions
void* threadFunction()
{
  // Loop as long as SIGINT isn't detected and the hardware button hasn't been pressed
  while(should_run && !hardware_button_pressed)
  {
  	// Check to see if the hardware button was pressed
  	checkHardwareButton();

  	// If the time has elapsed past the period time, read and print the temperature
  	if(should_generate_reports == 1 && time(0) >= last_read_time + period)
  	{
  	  readTemperature();
  	}
  }

  return NULL;
}

// Print the correct usage when the user inputs an incorrect argument
void printUsage()
{
  fprintf(stderr, "Correct usage: ./lab4b --period=# --scale=# --log=filename\n");
}

// Close the log file and the hardware sensors
void closeAll()
{
  // Close the log file
  if(log_flag == 1)
  {
    if(fclose(log_file)) 
	  {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
	  }
  }

  // Close the hardware button and temperature sensor
  mraa_gpio_close(hardware_button);
  mraa_aio_close(temperature_sensor);
}

// MAIN //

int main(int argc, char** argv) 
{
  // Struct with the available long options
  static struct option long_options[] =
  {
    { "period", required_argument, 0, 'p' },
    { "scale",  required_argument, 0, 's' },
    { "log",    required_argument, 0, 'l' },
    { 0,        0,                 0,  0  }
  };

  // Loop through options
  int getOptStatus = 0;
  while((getOptStatus = getopt_long(argc, argv, "", long_options, NULL)) != -1)
  {
  	// Switch to process user inputs
    switch(getOptStatus)
    {
	  case 'p':
		  if(strcmp(optarg, "") != 0)
		  {
		    unsigned int i;
		    for(i = 0; i < strlen(optarg); i++)
		    {
		  	  if(!isdigit(optarg[i]))
		  	  {
		  	    fprintf(stderr, "Error: period is not a valid integer: %s\n", optarg);
		  	    exit(1);
		  	  }
		    }
	      period = atoi(optarg);
	      if(period < 0)
	      {
	      	fprintf(stderr, "Error: period must be positive\n");
	      	exit(1);
	      }
		  }
		  break;
	  case 's':
		  if(strcmp(optarg, "") != 0)
	   	{
		    if(strlen(optarg) == 1 && (optarg[0] == 'C' || optarg[0] == 'F'))
        {
	        scale = optarg[0];
        }
	      else
	      {
	        fprintf(stderr, "Error: incorrect scale input: try C or F\n");
	        exit(1);
	      }
	    }
	    break;
	  case 'l':
	  	log_filename = optarg;
	  	log_flag = 1;
	    break;
	  default:
		  // Print correct usage when finding an unrecognized argument
		  printUsage();
		  exit(1);
	    break;
	  }
  }

  // If the log flag is high
  if(log_flag == 1)
  {
  	// Open the file
  	log_file = fopen(log_filename, "w");

  	// Check if the file was created successfully
	  if(log_file == NULL)
	  {
	    fprintf(stderr, "%s\n", strerror(errno));
	    exit(1);
	  }
  }

  // Initialize the temperature sensor and hardware button
  temperature_sensor = mraa_aio_init(0);
  hardware_button = mraa_gpio_init(3);
  mraa_gpio_dir(hardware_button, MRAA_GPIO_IN);
  setenv("TZ", "PST8PDT", true);
  tzset();
  signal(SIGINT, handler);

  // Check the hardware button
  checkHardwareButton();

  // Read the temperature
  readTemperature();

  // Create a single worker thread to print the temperature
  int create_status = pthread_create(&thread, NULL, threadFunction, NULL);
  if(create_status != 0)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Poll for user input while run flag is high
  while(should_run)
  {
  	// Initialize poll options for just the user input (STDIN)
    struct pollfd fds[1];
    memset(&fds, 0, sizeof(fds));
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN | POLLERR;

    // Grab the result of the poll call
    int pollStatus = poll(fds, 1, 0);

    // If the poll call was successful
	  if(pollStatus > 0)
	  {
	    // If the keyboard has input that is ready to be read
	    if(fds[0].revents & POLLIN)
	    {  
	      // Process the user input
	      processUserInput();
	    }

	    // If the keyboard's STDIN cannot be read from
	    if(fds[0].revents & POLLERR)
	    {
	  	  // If an error occurs, print an error message
	  	  if(log_flag == 1)
	  	  {
	  	    fprintf(log_file, "Error: cannot read from keyboard\n");
	  	  }

        // Close all and exit
        closeAll();
        exit(1);
	    }
	  }  
  }

  // Close all and exit
  closeAll();
  exit(0);
}