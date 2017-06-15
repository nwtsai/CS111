// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //

#include "SortedList.h"
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define _GNU_SOURCE

// GLOBAL VARIABLES //

// SortedList_t* and SortedListElement_t* variables
SortedList_t* lists;
SortedListElement_t* list_elements;
int list_length = 0;

// User inputs
int thread_count = 1;
int iteration_count = 1;
int run_count;
char sync_option;
int list_count = 1;

// Flags
int yield_flag = 0;
int sync_flag = 0;
int yield_i = 0;
int yield_d = 0;
int yield_l = 0;

// Initialize opt_yield to 0
int opt_yield = 0;

// Timing variables
struct timespec begin;
struct timespec end;

// Locks
pthread_mutex_t* locks;
int* spin_locks;

// Position index for sublists
int* list_index;

// Wait-for-lock time array
long* wait_for_lock;
long total_wait_for_lock_time = 0;

// HELPER FUNCTION DECLARATIONS //

void determineYieldOptions(char* optarg);
void determineSyncOptions(char* optarg);
void initializeSublists();
void createRandomString(char* str, const int length);
void createRandomKeys();
void initializeLocks();
int calculateIndexPosition(const char* key);
void lockInsert(int i, int thread_id);
void spinInsert(int i, int thread_id);
void lockAddLengths(int i, int thread_id);
void spinAddLengths(int i, int thread_id);
void lockLookupDelete(int i, int thread_id);
void spinLookupDelete(int i, int thread_id);
void* worker(void* thread_id);
char* getTestName();
void printUsage();
void printYieldUsage();
void printSyncUsage();

// HELPER FUNCTION IMPLEMENTATIONS //

// Determine which options to set for opt_yield based on the user input
void determineYieldOptions(char* optarg)
{
  char current; 
  int i;
  for(i = 0; *(optarg + i) != '\0'; i++)
  {
    current = *(optarg + i);
    if(current == 'i')
    {
      yield_i = 1;
      opt_yield |= INSERT_YIELD;
    }
    else if(current == 'd')
    {
      yield_d = 1;
      opt_yield |= DELETE_YIELD;
    }
    else if (current == 'l')
    {
      yield_l = 1;
      opt_yield |= LOOKUP_YIELD;
    }
    else
    {
      printYieldUsage();
      exit(1);
    }
  }
}

// Determine which options to set for sync based on the user input
void determineSyncOptions(char* optarg)
{
  if(strlen(optarg) == 1 && (optarg[0] == 'm' || optarg[0] == 's'))
    sync_option = optarg[0];
  else
  {
    printSyncUsage();
    exit(1);
  }
}

// Initialize all the sub lists deterined by the user input
void initializeSublists()
{
  int i;
  lists = (SortedList_t*)malloc(sizeof(SortedList_t) * list_count);
  for(i = 0; i < list_count; i++)
  {
    lists[i].prev = &lists[i];
    lists[i].next = &lists[i];
    lists[i].key = NULL;
  }
}

// Create a random string of specified length specified and store it into the pointer passed in
void createRandomString(char* str, const int length) 
{
  int i;
  static const char lettersAndNumbers[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  for (i = 0; i < length; i++) 
    str[i] = lettersAndNumbers[rand() % (sizeof(lettersAndNumbers) - 1)];
  str[length] = 0;
}

// Create random keys for each element in the lists
void createRandomKeys()
{
  int i;

  // Seed
  srand(time(NULL));

  // Generate a random key for each run
  for(i = 0; i < run_count; i++)
  {
    int randomLength = rand() % 20 + 5; 
    char* randomTempKey = (char*)malloc((randomLength + 1) * sizeof(char));
    createRandomString(randomTempKey, randomLength);
    list_elements[i].key = randomTempKey;
  }
}

// Initialize the mutexes and spin locks depending on the number of lists
void initializeLocks()
{
  int i;

  // Check if the sync flag is high 
  if(sync_flag == 1)
  {
    // Initialize the mutexes
    if(sync_option == 'm')
    {
      locks = (pthread_mutex_t*)malloc(list_count * sizeof(pthread_mutex_t));
      for(i = 0; i < list_count; i++)
      {
        int mutexInitStatus = pthread_mutex_init(&locks[i], NULL);
        if(mutexInitStatus != 0)
        {
          fprintf(stderr, "%s\n", strerror(errno));
          exit(1);
        }
      }
    }

    // Initialize the spin lock test and set values to 0
    else if(sync_option == 's')
    {
      spin_locks = (int*)malloc(list_count * sizeof(int));
      memset(spin_locks, 0, list_count * sizeof(int));
    }
  } 
}

// Calculate the initial positions of each sublist
int calculateIndexPosition(const char* key) 
{
  int hash = 0;
  int i;
  for(i = 0; i < strlen(key); i++) 
  {
    hash += key[i];
  }
  int index_position = hash % list_count;
  return index_position;
}

// Use mutex locking to insert an element into the list
void lockInsert(int i, int thread_id)
{
  // Timing structs
  struct timespec begin_wait_time;
  struct timespec end_wait_time;

  // Begin timer for this thread waiting for the lock
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin_wait_time);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Attempt to grab the mutex
  pthread_mutex_lock(&locks[list_index[i]]);

  // End timer for this thread waiting for the lock
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end_wait_time);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate the time it took to wait for the lock, and add it to this thread's time
  long elapsed_seconds = end_wait_time.tv_sec - begin_wait_time.tv_sec;
  long elapsed_nanoseconds = end_wait_time.tv_nsec - begin_wait_time.tv_nsec;

  // Convert seconds to nanoseconds, and then append result to per thread total
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;
  wait_for_lock[thread_id] += elapsed_time;

  // Insert the element into the list
  SortedList_insert(&lists[list_index[i]], &list_elements[i]); 

  // Release the mutex
  pthread_mutex_unlock(&locks[list_index[i]]);
}

// Spin lock to insert an element into the list
void spinInsert(int i, int thread_id)
{
  // Timing structs
  struct timespec begin_wait_time;
  struct timespec end_wait_time;

  // Begin timer for this thread waiting for the lock
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin_wait_time);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Spin until the critical section is ready to be entered
  while(__sync_lock_test_and_set(&spin_locks[list_index[i]], 1));

  // End timer for this thread waiting for the lock
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end_wait_time);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate the time it took to wait for the lock, and add it to this thread's time
  long elapsed_seconds = end_wait_time.tv_sec - begin_wait_time.tv_sec;
  long elapsed_nanoseconds = end_wait_time.tv_nsec - begin_wait_time.tv_nsec;

  // Convert seconds to nanoseconds, and then append result to per thread total
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;
  wait_for_lock[thread_id] += elapsed_time;

  // Insert the element into the list
  SortedList_insert(&lists[list_index[i]], &list_elements[i]);

  // Release the spin lock 
  __sync_lock_release(&spin_locks[list_index[i]]);
}

// Use locks to add to the global length variable
void lockAddLengths(int i, int thread_id)
{
  // Timing structs
  struct timespec begin_wait_time;
  struct timespec end_wait_time;

  // Begin timer for this thread waiting for the lock
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin_wait_time);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Attempt to grab the mutex
  pthread_mutex_lock(&locks[i]);

  // End timer for this thread waiting for the lock
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end_wait_time);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate the time it took to wait for the lock, and add it to this thread's time
  long elapsed_seconds = end_wait_time.tv_sec - begin_wait_time.tv_sec;
  long elapsed_nanoseconds = end_wait_time.tv_nsec - begin_wait_time.tv_nsec;

  // Convert seconds to nanoseconds, and then append result to per thread total
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;
  wait_for_lock[thread_id] += elapsed_time;

  // Add to the list length
  list_length += SortedList_length(&lists[i]);

  // Release the mutex
  pthread_mutex_unlock(&locks[i]);
}

// Use spin locks to add to the global length variable
void spinAddLengths(int i, int thread_id)
{
  // Timing structs
  struct timespec begin_wait_time;
  struct timespec end_wait_time;

  // Begin timer for this thread waiting for the lock
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin_wait_time);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Spin until the critical section is ready to be entered
  while(__sync_lock_test_and_set(&spin_locks[i], 1));

  // End timer for this thread waiting for the lock
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end_wait_time);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate the time it took to wait for the lock, and add it to this thread's time
  long elapsed_seconds = end_wait_time.tv_sec - begin_wait_time.tv_sec;
  long elapsed_nanoseconds = end_wait_time.tv_nsec - begin_wait_time.tv_nsec;

  // Convert seconds to nanoseconds, and then append result to per thread total
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;
  wait_for_lock[thread_id] += elapsed_time;

  // Add to the list length
  list_length += SortedList_length(&lists[i]);

  // Release the spin lock
  __sync_lock_release(&spin_locks[i]);
}

// Use mutex locking to lookup and delete an element from the list
void lockLookupDelete(int i, int thread_id)
{
  // Timing structs
  struct timespec begin_wait_time;
  struct timespec end_wait_time;

  // Begin timer for this thread waiting for the lock
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin_wait_time);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Attempt to grab the mutex
  pthread_mutex_lock(&locks[list_index[i]]);

  // End timer for this thread waiting for the lock
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end_wait_time);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate the time it took to wait for the lock, and add it to this thread's time
  long elapsed_seconds = end_wait_time.tv_sec - begin_wait_time.tv_sec;
  long elapsed_nanoseconds = end_wait_time.tv_nsec - begin_wait_time.tv_nsec;

  // Convert seconds to nanoseconds, and then append result to per thread total
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;
  wait_for_lock[thread_id] += elapsed_time;

  // Look up an element
  SortedListElement_t* element = SortedList_lookup(&lists[list_index[i]], list_elements[i].key);
  if(element == NULL)
  {
    fprintf(stderr, "Error: Unable to look up element\n");
    exit(2);
  }

  // Delete the element
  int deleteStatus = SortedList_delete(element);
  if(deleteStatus != 0)
  {
    fprintf(stderr, "Error: Unable to delete element\n");
    exit(2);
  }

  // Release the mutex
  pthread_mutex_unlock(&locks[list_index[i]]);
}

// Spin lock to lookup and delete an element from the list
void spinLookupDelete(int i, int thread_id)
{
  // Timing structs
  struct timespec begin_wait_time;
  struct timespec end_wait_time;

  // Begin timer for this thread waiting for the lock
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin_wait_time);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Spin until the critical section is ready to be entered
  while(__sync_lock_test_and_set(&spin_locks[list_index[i]], 1));

  // End timer for this thread waiting for the lock
  int end_status = clock_gettime(CLOCK_MONOTONIC, &end_wait_time);
  if(end_status < 0) 
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Calculate the time it took to wait for the lock, and add it to this thread's time
  long elapsed_seconds = end_wait_time.tv_sec - begin_wait_time.tv_sec;
  long elapsed_nanoseconds = end_wait_time.tv_nsec - begin_wait_time.tv_nsec;

  // Convert seconds to nanoseconds, and then append result to per thread total
  long elapsed_time = elapsed_seconds * 1000000000L + elapsed_nanoseconds;
  wait_for_lock[thread_id] += elapsed_time;

  // Look up an element
  SortedListElement_t* element = SortedList_lookup(&lists[list_index[i]], list_elements[i].key);
  if(element == NULL)
  {
    fprintf(stderr, "Error: Unable to look up element\n");
    exit(2);
  }

  // Delete the element
  int deleteStatus = SortedList_delete(element);
  if(deleteStatus != 0)
  {
    fprintf(stderr, "Error: Unable to delete element\n");
    exit(2);
  }

  // Release the spin lock
  __sync_lock_release(&spin_locks[list_index[i]]);
}

// Worker thread algorithm
void* worker(void* thread_id)
{
  // Convert the argument to an int
  int thread_int = *(int*)thread_id;

  // Re-usable counter for the for loops
  int i;

  // Insert an element into the list
  for(i = thread_int; i < run_count; i += thread_count)
  {
    if(sync_option == 'm')
      lockInsert(i, thread_int);
    else if(sync_option == 's')
      spinInsert(i, thread_int);
    else
      SortedList_insert(&lists[list_index[i]], &list_elements[i]);
  }

  // Get the total list length
  list_length = 0;
  for(i = 0; i < list_count; i++)
  {
    if(sync_option == 'm')
      lockAddLengths(i, thread_int);
    else if(sync_option == 's')
      spinAddLengths(i, thread_int);
    else
      list_length += SortedList_length(&lists[i]);
  }
  
  // Lookup and remove elements from the list
  for(i = thread_int; i < run_count; i += thread_count)
  {
    if(sync_option == 'm')
      lockLookupDelete(i, thread_int);
    else if(sync_option == 's')
      spinLookupDelete(i, thread_int);
    else
    {
      // Look up an element
      SortedListElement_t* element = SortedList_lookup(&lists[list_index[i]], list_elements[i].key);
      if(element == NULL)
      {
        fprintf(stderr, "Error: Unable to look up element\n");
        exit(2);
      }

      // Delete the element
      int deleteStatus = SortedList_delete(element);
      if(deleteStatus != 0)
      {
        fprintf(stderr, "Error: Unable to delete element\n");
        exit(2);
      }
    }
  }

  return NULL;
}

// Grab the name of the test based on user options
char* getTestName()
{
  // Return value
  char* ret;

  // If statements for all possible cases
  if(yield_flag == 0)
  {
    if(sync_flag == 1)
    {
      if(sync_option == 'm')
        ret = "list-none-m";
      else if(sync_option == 's')
        ret = "list-none-s";
    }
    else
      ret = "list-none-none";
  }
  else
  {
    if(sync_flag == 1)
    {
      if(sync_option == 'm')
      {
        if(yield_i == 1 && yield_d == 1 && yield_l == 1)
          ret = "list-idl-m";
        else if(yield_d == 1 && yield_l == 1)
          ret = "list-dl-m";
        else if(yield_i == 1 && yield_l == 1)
          ret = "list-il-m";
        else if(yield_i == 1 && yield_d == 1)
          ret = "list-id-m";
        else if(yield_l == 1)
          ret = "list-l-m";
        else if(yield_d == 1)
          ret = "list-d-m";
        else if(yield_i == 1)
          ret = "list-i-m";
      }
      else if(sync_option == 's')
      {
        if(yield_i == 1 && yield_d == 1 && yield_l == 1)
          ret = "list-idl-s";
        else if(yield_d == 1 && yield_l == 1)
          ret = "list-dl-s";
        else if(yield_i == 1 && yield_l == 1)
          ret = "list-il-s";
        else if(yield_i == 1 && yield_d == 1)
          ret = "list-id-s";
        else if(yield_l == 1)
          ret = "list-l-s";
        else if(yield_d == 1)
          ret = "list-d-s";
        else if(yield_i == 1)
          ret = "list-i-s";
      }
    }
    else
    {
      if(yield_i == 1 && yield_d == 1 && yield_l == 1)
        ret = "list-idl-none";
      else if(yield_d == 1 && yield_l == 1)
        ret = "list-dl-none";
      else if(yield_i == 1 && yield_l == 1)
        ret = "list-il-none";
      else if(yield_i == 1 && yield_d == 1)
        ret = "list-id-none";
      else if(yield_l == 1)
        ret = "list-l-none";
      else if(yield_d == 1)
        ret = "list-d-none";
      else if(yield_i == 1)
        ret = "list-i-none";
    }
  }
  return ret;
}

// Print the correct usage when the user inputs an incorrect argument
void printUsage()
{
  fprintf(stderr, "Correct usage: ./lab2_list --threads=# --iterations=# --yield=[idl] --lists=#\n");
}

// Print the correct usage of the yield arguments when user inputs an incorrect argument
void printYieldUsage()
{
  fprintf(stderr, "Incorrect yield option: i = insert yield, d = delete yield, l = lookup yield\n");
}

// Print the correct usage of the sync argument when user inputs an incorrect argument
void printSyncUsage()
{
  fprintf(stderr, "Incorrect sync option: m = mutex, s = spin-lock\n");
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
    { "yield",      required_argument, 0, 'y' },
    { "sync",       required_argument, 0, 's' },
    { "lists",      required_argument, 0, 'l' },
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
        determineYieldOptions(optarg);
        break;
      case 's':
        sync_flag = 1;
        determineSyncOptions(optarg);
        break;
      case 'l':
        if(strcmp(optarg, "") != 0)
          list_count = atoi(optarg);
        break;
      default:
	      // Print correct usage when finding an unrecognized argument
	      printUsage();
	      exit(1);
        break;
    }
  }

  // Calculate the run count
  run_count = thread_count * iteration_count;

  // Create lists, number of lists depends on user input of --lists=# option
  initializeSublists();

  // Allocate space for all elements on heap
  list_elements = (SortedListElement_t*)malloc(run_count * sizeof(SortedListElement_t));

  // Create random keys for all the elements in the list 
  createRandomKeys();

  // Initialize the mutexes and spin locks
  initializeLocks();

  // Allocate space for threads on heap
  pthread_t* thread_pointer = (pthread_t*)malloc(thread_count * sizeof(pthread_t));

  // Create thread IDs on heap
  int* thread_ids = (int*)malloc(thread_count * sizeof(int));

  // Create an array of all the per-thread wait-for-lock times, and set initial values to 0
  wait_for_lock = (long*)malloc(thread_count * sizeof(long));
  memset(wait_for_lock, 0, thread_count * sizeof(long));

  int i;

  // Determine where each sublist starts
  list_index = (int*)malloc(run_count * sizeof(int));
  for(i = 0; i < run_count; i++)
  {
    list_index[i] = calculateIndexPosition(list_elements[i].key);
  }

  // Begin timer
  int begin_status = clock_gettime(CLOCK_MONOTONIC, &begin);
  if(begin_status < 0) 
  { 
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  // Spawn all threads
  for(i = 0; i < thread_count; i++)
  {
    thread_ids[i] = i;
    int createStatus = pthread_create(thread_pointer + i, NULL, worker, &thread_ids[i]);
    if(createStatus != 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  // Wait for all threads to finish
  for(i = 0; i < thread_count; i++)
  {
    int joinStatus = pthread_join(thread_pointer[i], NULL);
    if(joinStatus != 0)
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
  int operation_count = 3 * thread_count * iteration_count;

  // Calculate the cost per operation
  long average_time_per_operation = elapsed_time / operation_count;

  // Add up the wait-for-lock times for each thread
  for(i = 0; i < thread_count; i++)
  {
    // Add all per-thread wait-for-lock times into one variable
    total_wait_for_lock_time += wait_for_lock[i];
  }

  // Number of locks
  long lock_count = 3 * thread_count * iteration_count;
  long average_wait_for_lock_time = total_wait_for_lock_time / lock_count;

  // Calculate the list length after all threads have exited
  list_length = 0;
  for(i = 0; i < list_count; i++)
  {
    list_length += SortedList_length(&lists[i]);
  }

  // Check the length of the list to confirm that it is 0, and exit otherwise
  if(list_length != 0)
  {
    fprintf(stderr, "Error: list length is not 0. Actual list length: %d\n", list_length);
    exit(2);
  }

  // Grab the test name from user options
  char* test_name = getTestName();

  // Print out the output in the specified format so that we can easily graph the results with gnuplot
  fprintf(stdout, "%s,%d,%d,%d,%d,%ld,%ld,%ld\n", test_name, thread_count, iteration_count, list_count, operation_count, elapsed_time, average_time_per_operation, average_wait_for_lock_time);

  // Free heap space
  free(thread_ids);
  free(thread_pointer);
  free(list_elements);
  free(lists);
  free(locks);
  free(spin_locks);
  free(list_index);
  free(wait_for_lock);

  // Exit normally
  exit(0);
}
