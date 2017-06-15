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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>

// GLOBAL VARIABLES //

// B constant
static const int B_CONST = 4275;

// Period value
uint32_t period = 1;

// Hardware variable
mraa_aio_context temperature_sensor;

// Variable remains true as long as SIGINT doesn't happen
static sig_atomic_t volatile should_run = true;

// Variables that read user input
char* id = NULL;
char* host;
int port_num = -1;
sig_atomic_t volatile scale = 'F';
int log_flag = 0;
int socket_flag = 0;
FILE* log_file = NULL;
char* log_filename = NULL;

// Host variables
int client_socket_fd;
struct sockaddr_in server_address;
struct hostent* host_server;

// Time variable
time_t last_read_time;

// Thread variable
pthread_t thread;

// Flag for generating reports
int should_generate_reports = 1;

// HELPER FUNCTION DECLARATIONS //

void handler();
void processInput(int file_descriptor);
void readTemperature();
void* threadFunction();
void printUsage();
void closeAll(int exit_code);

// HELP FUNCTION IMPLEMENTATIONS //

// Set the flag to 0 when the handler detects a SIGINT (^C)
void handler()
{
  should_run = 0;
}

// Process the user input
void processInput(int file_descriptor)
{
  // String for the user input
  char user_input[1024];

  ssize_t num_bytes; 
  num_bytes = read(file_descriptor, user_input, 255);
  if(num_bytes < 0)
  {
    if(log_flag == 1)
    {
      fprintf(stderr, "Error: Could not process command from server\n");
      closeAll(2);
    }
  }
  
  // Check if the user input matches any recognized commands
  if(strncmp(user_input, "OFF\n", 4) == 0)
  {
    fprintf(log_file, "OFF\n");
    fflush(log_file);

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
      fflush(log_file);
    }

    // If the socket flag is enabled, send the SHUTDOWN command to the server
    if(socket_flag == 1)
    {
      dprintf(client_socket_fd, "%s SHUTDOWN\n", time_stamp);
    }

	  // Close all and exit
	  closeAll(0);
  }
  else if(strncmp(user_input, "STOP\n", 5) == 0)
  {
    fprintf(log_file, "STOP\n");
    fflush(log_file);
  	should_generate_reports = 0;
  }
  else if(strncmp(user_input, "START\n", 6) == 0)
  {
    fprintf(log_file, "START\n");
    fflush(log_file);
  	should_generate_reports = 1;
  }
  else if(strncmp(user_input, "SCALE=F\n", 8) == 0)
  {
    fprintf(log_file, "SCALE=F\n");
    fflush(log_file);
  	scale = 'F';
  }
  else if(strncmp(user_input, "SCALE=C\n", 8) == 0)
  {
    fprintf(log_file, "SCALE=C\n");
    fflush(log_file);
  	scale = 'C';
  }
  else if(strncmp(user_input, "PERIOD=", 7) == 0)
  {
    uint32_t temp_period;
    if(sscanf(user_input, "PERIOD=%d\n", &temp_period) == EOF) 
    {
      if(log_flag == 1)
      {
        fprintf(log_file, "Error: Unrecognized command:%s\n", user_input);
        fflush(log_file);
      }
      closeAll(1);
    };
    fprintf(log_file, "PERIOD=%d\n", temp_period);
    fflush(log_file);
    period = temp_period;
  }
  else
  {
  	if(log_flag == 1)
  	{
  	  fprintf(log_file, "Error: Unrecognized command: %s\n", user_input);
      fflush(log_file);
  	}
  	closeAll(1);
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
    // Send the data to the log file if the log flag is high
    if(log_flag == 1)
    {
      fprintf(log_file, "%s 0%.1f\n", time_stamp, temperature);
      fflush(log_file);
    }

    // Send the data to the socket if the socket flag is high
    if(socket_flag == 1)
    {
      dprintf(client_socket_fd, "%s 0%.1f\n", time_stamp, temperature);
    }
  }
  else
  {
    // Send the data to the log file if the log flag is high
    if(log_flag == 1)
    {
      fprintf(log_file, "%s %.1f\n", time_stamp, temperature);
      fflush(log_file);
    }

    // Send the data to the socket if the socket flag is high
    if(socket_flag == 1)
    {
      dprintf(client_socket_fd, "%s %.1f\n", time_stamp, temperature);
    }
  }
}

// Thread checks hardware button and reads temperature under the specified conditions
void* threadFunction()
{
  // Loop as long as SIGINT isn't detected and the hardware button hasn't been pressed
  while(should_run)
  {
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
  fprintf(stderr, "Correct usage: ./lab4c --id=# --host=# --log=filename\n");
}

// Close the log file and the hardware sensors
void closeAll(int exit_code)
{
  // Close the log file
  if(log_flag == 1)
  {
    if(fclose(log_file)) 
	  {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(2);
	  }
  }

  // Close the temperature sensor
  mraa_aio_close(temperature_sensor);

  // Exit with the specified exit code
  exit(exit_code);
}

// MAIN //

int main(int argc, char** argv) 
{
  // Struct with the available long options
  static struct option long_options[] =
  {
    { "id",   required_argument, 0, 'i' },
    { "host", required_argument, 0, 'h' },
    { "log",  required_argument, 0, 'l' },
    { 0,      0,                 0,  0  }
  };

  int option_index = 0;

  // Loop through options
  while(option_index < argc)
  {
    int getOptStatus = 0;
    if((getOptStatus = getopt_long(argc, argv, "", long_options, NULL)) != -1)
    {
  	  // Switch to process user inputs
      switch(getOptStatus)
      {
	    case 'i':
		    if(strcmp(optarg, "") != 0)
		    {
	        id = optarg;
          if(strlen(id) != 9)
          {
            fprintf(stderr, "Error: Invalid ID: %s, should be 9 digits long\n", id);
            closeAll(1);
          }
		    }
		    break;
	    case 'h':
		    if(strcmp(optarg, "") != 0)
	   	  {
		      host = optarg;
	      }
	      break;
	    case 'l':
	  	  log_filename = optarg;
	  	  log_flag = 1;
	      break;
	    default:
		    // Print correct usage when finding an unrecognized argument
		    printUsage();
        closeAll(1);
	      break;
	    }
    }
    else
    {
      port_num = atoi(argv[option_index]);
    }
    option_index++;
  }

  // If no port number is given, print an error and exit
  if(port_num == -1)
  {
    fprintf(stderr, "Error: No port number given\n");
    closeAll(1);
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
      closeAll(2);
	  }
  }

  // Set up the host
  if(id != NULL && host != NULL && port_num != -1)
  {
    host_server = gethostbyname(host);
    client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    memcpy((char*)&server_address.sin_addr.s_addr, (char*)host_server->h_addr, host_server->h_length);
    server_address.sin_port = htons(port_num);
    int connectStatus = connect(client_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address));
    if(connectStatus < 0)
    { 
      fprintf(stderr, "Error: Invalid host or port number\n");
      closeAll(1);
    }
    else 
    {
      socket_flag = 1;
    }
    dprintf(client_socket_fd, "ID=%s\n", id);
  }
  else
  {
    fprintf(stderr, "Error: Host or port number not specified\n");
    closeAll(1);
  }

  // Initialize the temperature sensor and hardware button
  temperature_sensor = mraa_aio_init(0);
  setenv("TZ", "PST8PDT", true);
  tzset();
  signal(SIGINT, handler);

  // Create a single worker thread to print the temperature
  int create_status = pthread_create(&thread, NULL, threadFunction, NULL);
  if(create_status != 0)
  {
    fprintf(stderr, "Error: %s\n", strerror(errno));
    closeAll(2);
  }

  // Initialize poll options for just the server
  struct pollfd fds[1];
  fds[0].fd = client_socket_fd;
  fds[0].events = POLLIN | POLLERR;

  // Poll for server input while run flag is high
  while(should_run)
  {
    // Grab the result of the poll call
    int pollStatus = poll(fds, 1, 0);

    // If the poll call was successful
	  if(pollStatus > 0)
	  {
	    // If the server sends input that is ready to be read
	    if(fds[0].revents & POLLIN)
	    {  
	      // Process the server's input
	      processInput(client_socket_fd);
	    }

	    // If the server cannot be read from
	    if(fds[0].revents & POLLERR)
	    {
	  	  // If an error occurs, print an error message
	  	  if(log_flag == 1)
	  	  {
	  	    fprintf(log_file, "Error: cannot read command from server\n");
          fflush(log_file);
	  	  }

        // Close all and exit
        closeAll(2);
	    }
	  }  
  }

  // Close all and exit
  closeAll(0);
}