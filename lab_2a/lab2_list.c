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
SortedList_t* list;
SortedListElement_t* list_elements;
int list_length = 0;

// User inputs
int thread_count = 1;
int iteration_count = 1;
int run_count;
char sync_option;

// Flags
int yield_flag = 0;
int sync_flag = 0;
int yield_i = 0;
int yield_d = 0;
int yield_l = 0;

// Set opt_yield to 0
int opt_yield = 0;

// Timing variables
struct timespec begin;
struct timespec end;

// Lock
pthread_mutex_t lock;

// Test and set value
int testAndSetValue = 0;

// HELPER FUNCTION DECLARATIONS //

void determineYieldOptions(char* optarg);
void determineSyncOptions(char* optarg);
void createRandomString(char* str, const int length);
void createRandomKeys();
void lockUnlockInsert(int i);
void spinLockInsert(int i);
void lockUnlockLookupDelete(int i);
void spinLockLookupDelete(int i);
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
  for(int i = 0; *(optarg + i) != '\0'; i++)
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

// Create a random string of specified length specified and store it into the pointer passed in
void createRandomString(char* str, const int length) 
{
    static const char lettersAndNumbers[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < length; i++) 
        str[i] = lettersAndNumbers[rand() % (sizeof(lettersAndNumbers) - 1)];
    str[length] = 0;
}

// Create random keys for each element in the lists
void createRandomKeys()
{
  // Seed
  srand(time(NULL));

  // Generate a random key for each run
  for(int i = 0; i < run_count; i++)
  {
    int randomLength = rand() % 20 + 5; 
    char* randomTempKey = (char*)malloc((randomLength + 1) * sizeof(char));
    createRandomString(randomTempKey, randomLength);
    list_elements[i].key = randomTempKey;
  }
}

// Use mutex locking to insert an element into the list
void lockUnlockInsert(int i)
{
  pthread_mutex_lock(&lock);
  SortedList_insert(list, &list_elements[i]); 
  pthread_mutex_unlock(&lock);
}

// Spin lock to insert an element into the list
void spinLockInsert(int i)
{
  while(__sync_lock_test_and_set(&testAndSetValue, 1));
  SortedList_insert(list, &list_elements[i]);
  __sync_lock_release(&testAndSetValue);
}

// Use mutex locking to lookup and delete an element from the list
void lockUnlockLookupDelete(int i)
{
  pthread_mutex_lock(&lock);
  SortedListElement_t* element = SortedList_lookup(list, list_elements[i].key);
  if(element == NULL)
  {
    fprintf(stderr, "Error: Unable to look up element\n");
    exit(2);
  }
  int deleteStatus = SortedList_delete(element);
  if(deleteStatus != 0)
  {
    fprintf(stderr, "Error: Unable to delete element\n");
    exit(2);
  }
  pthread_mutex_unlock(&lock);
}

// Spin lock to lookup and delete an element from the list
void spinLockLookupDelete(int i)
{
  while(__sync_lock_test_and_set(&testAndSetValue, 1));
  SortedListElement_t* element = SortedList_lookup(list, list_elements[i].key);
  if(element == NULL)
  {
    fprintf(stderr, "Error: Unable to look up element\n");
    exit(2);
  }
  int deleteStatus = SortedList_delete(element);
  if(deleteStatus != 0)
  {
    fprintf(stderr, "Error: Unable to delete element\n");
    exit(2);
  }
  __sync_lock_release(&testAndSetValue);
}

// Worker thread algorithm
void* worker(void* thread_id)
{
  // Re-usable counter for the for loops
  int i = 0;

  // Insert an element into the list
  for(i = *(int*)thread_id; i < run_count; i += thread_count)
  {
    if(sync_option == 'm')
      lockUnlockInsert(i);
    else if(sync_option == 's')
      spinLockInsert(i);
    else
      SortedList_insert(list, &list_elements[i]);
  }

  // Get the list length
  list_length = SortedList_length(list);
  
  // Lookup and remove elements from the list
  for(i = *(int*)thread_id; i < run_count; i += thread_count)
  {
    if(sync_option == 'm')
      lockUnlockLookupDelete(i);
    else if(sync_option == 's')
      spinLockLookupDelete(i);
    else
    {
      SortedListElement_t* element = SortedList_lookup(list, list_elements[i].key);
      if(element == NULL)
      {
        fprintf(stderr, "Error: Unable to look up element\n");
        exit(2);
      }
      int deleteStatus = SortedList_delete(element);
      if(deleteStatus != 0)
      {
        fprintf(stderr, "Error: Unable to delete element\n");
        exit(2);
      }
    }
  }

  // Get the list length
  list_length = SortedList_length(list);

  return NULL;
}

// Grab the name of the test based on user options
char* getTestName()
{
  // Return value
  char* ret;

  // If statement for all possible cases
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
  fprintf(stderr, "Correct usage: ./lab2_list --threads=# --iterations=# --yield=[idl]\n");
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
      default:
	      // Print correct usage when finding an unrecognized argument
	      printUsage();
	      exit(1);
        break;
    }
  }

  // Calculate the run count
  run_count = thread_count * iteration_count;

  // Make an empty circular double linked list
  list = (SortedList_t*)malloc(sizeof(SortedList_t));
  list->prev = list;
  list->next = list;
  list->key = NULL;

  // Allocate space for all elements on heap
  list_elements = (SortedListElement_t*)malloc(run_count * sizeof(SortedListElement_t));

  // Create random keys for all the elements in the list 
  createRandomKeys();

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

  // Create thread IDs on heap
  int* thread_ids = (int*)malloc(thread_count * sizeof(int));

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
    thread_ids[i] = i;
    int createStatus = pthread_create(thread_pointer + i, NULL, worker, &thread_ids[i]);
    if(createStatus != 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  // Wait for all threads to finish
  for(int i = 0; i < thread_count; i++)
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

  // Check the length of the list to confirm that it is 0
  if(list_length != 0)
  {
    fprintf(stderr, "Error: List length is not 0\n");
    exit(2);
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

  // Grab the test name from user options
  char* test_name = getTestName();
  fprintf(stdout, "%s,%d,%d,1,%d,%ld,%ld\n", test_name, thread_count, iteration_count, operation_count, elapsed_time, average_time_per_operation);

  // Free heap space
  free(thread_ids);
  free(thread_pointer);
  free(list_elements);

  exit(0);
}
