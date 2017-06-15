// // // // // // // //
// Nathan Tsai       //
// nwtsai@gmail.com  //
// 304575323         //
// // // // // // // //

#include "SortedList.h"
#include <string.h>
#include <pthread.h>
#define _GNU_SOURCE

// Implement the insert function defined by interface
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
  // Return empty if it is an empty linked list
  if(list == NULL || element == NULL)
    return;

  // Initialize temp to the first node
  SortedListElement_t* temp = list->next;

  // Iterate unntil we find a node that has a value less than the temp value, as long as we don't loop around to the beginning again
  while(temp != list)
  {
    // Break if the temp key is greater than the element to insert's key
    if(strcmp(temp->key, element->key) > 0)
      break;

    // Move on to the next node
    temp = temp->next;
  }

  // Give up CPU if yield is high
  if(opt_yield & INSERT_YIELD)
    sched_yield();

  // Add element before the temp node
  element->next = temp;
  element->prev = temp->prev;
  temp->prev->next = element;
  temp->prev = element;
}

// Implement the delete function defined by interface
int SortedList_delete(SortedListElement_t *element)
{
  if(element == NULL)
    return 1;

  // Check if the list is corrupted
  if(element->prev->next == element->next->prev)
  {
    // Give up CPU if yield is high
    if(opt_yield & DELETE_YIELD)
      sched_yield();
    
    // Rearrange pointers to essentially delete the current element
    element->prev->next = element->next;
    element->next->prev = element->prev;
    return 0;
  }

  return 1;
}

// Implement the lookup function defined by interface
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
  if(list == NULL || key == NULL)
    return NULL;

  // Initialize temp to the first item
  SortedListElement_t* temp = list->next;

  // Iterate until we loop back around to beginning of the list
  while(temp != list)
  {
    // Return match if found
    if(strcmp(key, temp->key) == 0)
      return temp;

    // Give up CPU if yield is high
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();

    // Move on if match isn't found
    temp = temp->next;
  }

  // If no match is found
  return NULL;
}

// Implement the length function defined by interface
int SortedList_length(SortedList_t *list)
{
  // Check for corrupted list
  if(list == NULL)
  {
    return -1;
  }

  int length = 0;

  // Initialize temp to the first item
  SortedListElement_t* temp = list->next;

  while(temp != list)
  {
    // Increment the length
    length = length + 1;

    // Give up CPU if yield is high
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();

    // Move on to the next element
    temp = temp->next;
  }
  return length;
}