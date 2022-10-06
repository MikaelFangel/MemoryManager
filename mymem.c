#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>


/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

struct memoryList
{
  // doubly-linked list
  struct memoryList *prev;
  struct memoryList *next;

  int size;            // How many bytes in this block?
  char alloc;          // 1 if this block is allocated,
                       // 0 if this block is free.
  void *ptr;           // location of block in memory pool.
};

strategies myStrategy = NotSet;    // Current strategy


size_t mySize;
void *myMemory = NULL;

static struct memoryList *head;
static struct memoryList *tail;
//static struct memoryList *next;   Old from Bhupjit, guess it should be tail


/* initmem must be called prior to mymalloc and myfree.

   initmem may be called more than once in a given exeuction;
   when this occurs, all memory you previously malloc'ed  *must* be freed,
   including any existing bookkeeping data.

   strategy must be one of the following:
		- "best" (best-fit)
		- "worst" (worst-fit)
		- "first" (first-fit)
		- "next" (next-fit)
   sz specifies the number of bytes that will be available, in total, for all mymalloc requests.
*/

void initmem(strategies strategy, size_t sz)
{
	myStrategy = strategy;

	/* all implementations will need an actual block of memory to use */
	mySize = sz;

	if (myMemory != NULL) free(myMemory); /* in case this is not the first time initmem2 is called */

	/* TODO: release any other memory you were using for bookkeeping when doing a re-initialization! */
    if(head != NULL) {
        struct memoryList *current;
        current = head;
        while(current->next != NULL) {
            free(current->prev);
            current = current->next;
        }
        free(current);
    }

	myMemory = malloc(sz);
	
	/* TODO: Initialize memory management structure. */
    // Allocate the size of memoryList to head.
    head = (struct memoryList*) malloc(sizeof(struct memoryList));
    head->alloc = 0;        // Not allocated
    head->size = sz;        // Size of memory block
    head->ptr = myMemory;   // Point to the allocated memory block address
    head->next = NULL;      // Does not link to other blocks at init
    head->prev = NULL;
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1 
 */

void *mymalloc(size_t requested)
{
	assert((int)myStrategy > 0);
	
	switch (myStrategy)
	  {
	  case NotSet: 
	            return NULL;
	  case First:
	            return NULL;
	  case Best:
	            return NULL;
	  case Worst:
	            return NULL;
	  case Next:
	            return NULL;
	  }
	return NULL;
}


/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void* block)
{
	return;
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when refered to "memory" here, it is meant that the 
 * memory pool this module manages via initmem/mymalloc/myfree. 
 */

/* Get the number of contiguous areas of free space in memory. */
// Iterate list every block and count '0' allocs
int mem_holes() {
    int count = 0;
    struct memoryList *current = head;
    while(current->next != NULL) {
        if (current->alloc == '0') {
            count++;
        }
        current = current->next;
    }
	return count;
}

/* Get the number of bytes allocated */
// Might have misunderstood, but this is calculating allocated size and not the number of allocated blocks as guide suggests
int mem_allocated() {
    return mySize - mem_free();
}

/* Number of non-allocated bytes */
// Iterate list and count non-allocated size
int mem_free() {
    int size = 0;
    struct memoryList *current = head;
    while(current->next != NULL) {
        if (current->alloc == '0')
            size += current->size;
        current = current->next;
    }
	return size;
}

/* Number of bytes in the largest contiguous area of unallocated memory */
// Iterate list and track of largest unallocated block
int mem_largest_free() {
    int max = 0;
    struct memoryList *current = head;
    while(current->next != NULL) {
        if((current->alloc == '0') && (current->size > max))
            max = current->size;
        current = current->next;
    }
	return max;
}

/* Number of free blocks smaller than or equal to "size" bytes. */
// Iterate list and increment counter when block size < size
int mem_small_free(int size) {
	int count = 0;
    struct memoryList *current = head;
    while(current->next != NULL) {
        if((current->alloc == '0') && (current->size <= size))
            count++;
        current = current->next;
    }
    return count;
}       

// Is a byte allocated?
char mem_is_alloc(void *ptr) {
    return NULL;
}

/* 
 * Feel free to use these functions, but do not modify them.  
 * The test code uses them, but you may find them useful.
 */


//Returns a pointer to the memory pool.
void *mem_pool()
{
	return myMemory;
}

// Returns the total number of bytes in the memory pool. */
int mem_total()
{
	return mySize;
}


// Get string name for a strategy. 
char *strategy_name(strategies strategy)
{
	switch (strategy)
	{
		case Best:
			return "best";
		case Worst:
			return "worst";
		case First:
			return "first";
		case Next:
			return "next";
		default:
			return "unknown";
	}
}

// Get strategy from name.
strategies strategyFromString(char * strategy)
{
	if (!strcmp(strategy,"best"))
	{
		return Best;
	}
	else if (!strcmp(strategy,"worst"))
	{
		return Worst;
	}
	else if (!strcmp(strategy,"first"))
	{
		return First;
	}
	else if (!strcmp(strategy,"next"))
	{
		return Next;
	}
	else
	{
		return 0;
	}
}


/* 
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory()
{
	return;
}

/* Use this function to track memory allocation performance.  
 * This function does not depend on your implementation, 
 * but on the functions you wrote above.
 */ 
void print_memory_status()
{
	printf("%d out of %d bytes allocated.\n",mem_allocated(),mem_total());
	printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n",mem_free(),mem_holes(),mem_largest_free());
	printf("Average hole size is %f.\n\n",((float)mem_free())/mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char **argv) {
        strategies strat;
	void *a, *b, *c, *d, *e;
	if(argc > 1)
	  strat = strategyFromString(argv[1]);
	else
	  strat = First;
	
	
	/* A simple example.  
	   Each algorithm should produce a different layout. */
	
	initmem(strat,500);
	
	a = mymalloc(100);
	b = mymalloc(100);
	c = mymalloc(100);
	myfree(b);
	d = mymalloc(50);
	myfree(a);
	e = mymalloc(25);
	
	print_memory();
	print_memory_status();
	
}
