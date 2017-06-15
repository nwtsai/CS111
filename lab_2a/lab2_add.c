// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //

#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define _GNU_SOURCE

// GLOBAL VARIABLES //

// User inputs
int thread_count = 1;
int iteration_count = 1;
char sync_option;

// Flags
int yield_flag = 0;
int sync_flag = 0;

// Timing variables
struct timespec begin;
struct timespec end;

// Lock
pthread_mutex_t lock;

// Test and set value
int testAndSetValue = 0;

// HELPER FUNCTION DECLARATIONS //

void add(long long *pointer, long long value);
void lockUnlockAdd(long long *counter, long long value);
void spinLockAdd(long long *counter, long long value);
void compareSwapAdd(long long *counter, long long value);
void* worker(void* counter);
char* getTestName();
void printUsage();
void printSyncUsage();

// HELPER FUNCTION IMPLEMENTATIONS //

// Generic add function given in the specs
void add(long long *pointer, long long value)
{
  long long sum = *pointer + value;
  if(yield_flag)
    sched_yield();
  *pointer = sum;
}

// Lock then add to the counter
void lockUnlockAdd(long long *counter, long long value)
{
  pthread_mutex_lock(&lock);
  add(counter, value);
  pthread_mutex_unlock(&lock);
}

// Void spin-lock
void spinLockAdd(long long *counter, long long value)
{
  while(__sync_lock_test_and_set(&testAndSetValue, value));
  add(counter, value);
  __sync_lock_release(&testAndSetValue);
}

// Compare then swap to add to counter
void compareSwapAdd(long long *counter, long long value)
{
  long long prev;
  long long sum;
  do
  {
    prev = *counter;
    sum = prev + value;
    if(yield_flag)
      sched_yield();
  } while(__sync_val_compare_and_swap(counter, prev, sum) != prev);
}

// Each thread runs this adder function
void* worker(void* counter)
{
  // Add 1 iteration_count times
  for(int i = 0; i < iteration_count; i++)
  {
    // Mutex
    if(sync_option == 'm')
      lockUnlockAdd((long long*) counter, 1);

    // Spin-lock
    else if (sync_option == 's')
      spinLockAdd((long long*) counter, 1);

    // Compare and swap
    else if (sync_option == 'c')
      compareSwapAdd((long long*) counter, 1);

    // Non-synchronized option
    else
      add((long long*) counter, 1);
  }

  // Subtract 1 iteration_count times
  for(int i = 0; i < iteration_count; i++)
  {
    // Mutex
    if(sync_option == 'm')
      lockUnlockAdd((long long*) counter, -1);

    // Spin-lock
    else if (sync_option == 's')
      spinLockAdd((long long*) counter, -1);

    // Compare and swap
    else if (sync_option == 'c')
      compareSwapAdd((long long*) counter, -1);

    // Non-synchronized option
    else
      add((long long*) counter, -1);
  }

  return NULL;
}

// Grab the name of the test based on user options
char* getTestName()
{
  char* ret;
  if(yield_flag == 0)
  {
    if(sync_flag == 1)
    {
      if(sync_option == 'm')
        ret = "add-m";
      else if(sync_option == 's')
        ret = "add-s";
      else if(sync_option == 'c')
        ret = "add-c";
    }
    else
      ret = "add-none";
  }
  else
  {
    if(sync_flag == 1)
    {
      if(sync_option == 'm')
        ret = "add-yield-m";
      else if(sync_option == 's')
        ret = "add-yield-s";
      else if(sync_option == 'c')
        ret = "add-yield-c";
    }
    else
      ret = "add-yield-none";
  }
  return ret;
}

// Print the correct usage when the user inputs an incorrect argument
void printUsage()
{
  fprintf(stderr, "Correct usage: ./lab2_add --threads=# --iterations=# --yield\n");
}

// Print the correct usage of the sync argument when user inputs an incorrect argument
void printSyncUsage()
{
  fprintf(stderr, "Incorrect sync option: m = mutex, s = spin-lock, c = compare\n");
}

// MAIN //

int main(int argc, char** argv)
{ 
  int getOptStatus = 0;

  // Struct with the available long options
  static struct option long_options[] =
  {
    { "threads",    optional_argument, 0, 't' },
    { "iterations", optional_argument, 0, 'i' },
    { "yield",      no_argument,       0, 'y' },
    { "sync",       required_argument, 0, 's' },
    { 0,            0,                 0,  0  }
  };

  // Loop through options
  while((getOptStatus = getopt_long(argc, argv, "", long_options, NULL)) != -1)
  {
    switch(getOptStatus)
    {
      case 't':
        if(strcmp(optarg, "") != 0)
          thread_count = atoi(optarg);
        break;
      case 'i':
        if(strcmp(optarg, "") != 0)
          iteration_count = atoi(optarg);
        break;
      case 'y':
        yield_flag = 1;
        break;
      case 's':
        sync_flag = 1;
        if(strlen(optarg) == 1 && (optarg[0] == 'm' || optarg[0] == 's' || optarg[0] == 'c'))
          sync_option = optarg[0];
        else
        {
          printSyncUsage();
          exit(1);
        }
        break;
      default:
	      // Print correct usage when finding an unrecognized argument
	      printUsage();
	      exit(1);
        break;
    }
  }

  // Initialize the mutex
  if(sync_flag == 1 && sync_option == 'm')
  {
    int mutexInitStatus = pthread_mutex_init(&lock, NULL);
    if(mutexInitStatus != 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  // Allocate space for threads on heap
  pthread_t* thread_pointer = (pthread_t*)malloc(thread_count * sizeof(pthread_t));

  // Initialize long long counter to 0
  long long counter = 0;

  // Begin timer
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Spawn all threads
  for(int i = 0; i < thread_count; i++)
  {
    int create_status = pthread_create(thread_pointer + i, NULL, worker, &counter);
    if(create_status != 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  // Wait for all threads to finish
  for(int i = 0; i < thread_count; i++)
  {
    int join_status = pthread_join(*(thread_pointer + i), NULL);
    if(join_status != 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  // End timer
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate total elapsed time
  long elapsed_seconds = end.tv_sec - begin.tv_sec;
  long elapsed_nanoseconds = end.tv_nsec - begin.tv_nsec;

  // Convert seconds to nanoseconds, then add to the timed nanoseconds
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;

  // Calculate the number of operations
  int operation_count = 2 * thread_count * iteration_count;

  // Calculate the cost per operation
  long average_time_per_operation = elapsed_time / operation_count;

  // Grab the test name from user options
  char* test_name = getTestName();
  fprintf(stdout, "%s,%d,%d,%d,%ld,%ld,%lld\n", test_name, thread_count, iteration_count, operation_count, elapsed_time, average_time_per_operation, counter);

  exit(0);
}
