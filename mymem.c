#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>

/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

typedef struct memoryList
{
    // doubly-linked list
    struct memoryList *prev;
    struct memoryList *next;

    size_t size;
    bool alloc;

    void *ptr;
} memoryList;

void *allocate_block_of_memory(memoryList *blockPos, size_t requested_size);
memoryList *firstfit(size_t requested);
memoryList *find_block(void* block);
void merge_left(memoryList *block);
memoryList *worstfit(size_t requested);
memoryList *bestfit(size_t requested);

strategies myStrategy = NotSet;    // Current strategy


size_t mySize;
void *myMemory = NULL;

static memoryList *head;
static memoryList *tail;
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

    if (myMemory != NULL) {
        free(myMemory); /* in case this is not the first time initmem2 is called */
        myMemory = NULL;
    }

    /* TODO: release any other memory you were using for bookkeeping when doing a re-initialization! */
    memoryList *current = head;
    memoryList *next;
    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }


    /* all implementations will need an actual block of memory to use */
    mySize = sz;
    myMemory = malloc(sz);

    /* TODO: Initialize memory management structure. */
    head = (memoryList *) malloc(sizeof(memoryList));
    head->alloc = false;
    head->size = mySize;
    head->ptr = myMemory;
    head->next = NULL;
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
    // Check if there is a block large enough to hold the requested
    if(requested > mem_largest_free())
        return NULL;

    switch (myStrategy)
    {
        case NotSet: 
            return NULL;
        case First:
            return allocate_block_of_memory(firstfit(requested), requested);
        case Best:
            return allocate_block_of_memory(bestfit(requested), requested);
        case Worst:
            return allocate_block_of_memory(worstfit(requested), requested);
        case Next:
            return NULL;
        default:
            return NULL;
    }
}

void *allocate_block_of_memory(memoryList *blockPos, size_t requested_size)
{
    assert(blockPos->size >= requested_size);
    // Check the block recieved is a valid mem location
    if(blockPos == NULL)
        return NULL;

    // If the position given is equal to the requested size
    // then overtake the block and return a pointer
    if(blockPos->size == requested_size) {
        blockPos->alloc = true;
        return blockPos->ptr;
    }

    // Request block is smaller than the blockpos and we therefore
    // need to divide the memory into two
    memoryList *split_block = (memoryList *) malloc(sizeof(memoryList));

    // Update head if we takes its place with our split
    if(blockPos == head)
        head = split_block;

    // Setting values for the left side of our split block
    split_block->alloc = true;
    split_block->size = requested_size;
    split_block->ptr = blockPos->ptr;
    split_block->next = blockPos;
    split_block->prev = blockPos->prev;

    // Setting value for the right side of our split
    blockPos->size -= requested_size;
    blockPos->ptr = blockPos->ptr + requested_size;
    // Make sure to keep the linked list connected
    if(blockPos->prev != NULL)
        blockPos->prev->next = split_block;
    blockPos->prev = split_block;

    return split_block->ptr;
}

memoryList *firstfit(size_t requested) {
    memoryList *current = head;
    while(current != NULL) {
        if(!current->alloc && current->size >= requested)
            break;
        current = current->next;
    }

    return current;
}

memoryList *worstfit(size_t requested)
{
    memoryList *current, *maxptr;
    maxptr = head;

    // Find the first unallocated struct
    while (maxptr != NULL && maxptr->alloc)
        maxptr = maxptr->next;

    // Return NULL if we couldn't find a free space
    if (maxptr == NULL)
        return maxptr;

    current = maxptr;

    // Find the maximum sized free block
    while (current != NULL) {
        if (current->size > maxptr->size && !current->alloc)
            maxptr = current;
        current = current->next;
    }

    // Check if the requested size fit in the maximum sized block
    if (maxptr->size >= requested)
        return  maxptr;
    else
        return NULL;
}

memoryList *bestfit(size_t requested)
{
    memoryList *current = head;
    memoryList *bestfit = NULL;
    int smallestDiff = -1; // Start value

    while (current != NULL)
    {
        if(!current->alloc && current->size >= requested)
        {
            int diff = current->size - requested;

            // Set smallest diff
            if (smallestDiff == -1 || diff < smallestDiff)
            {
                smallestDiff = diff;
                bestfit = current;
            }
        }
        current = current->next;
    }

    // Return null if there is nothing found
    if (smallestDiff == -1)
        return NULL;

    return bestfit;
}

/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void* block)
{
    memoryList *list_block = find_block(block);
    if(list_block == NULL)
        return;

    // Freeing the only block in the memory list
    if(list_block->next == NULL && list_block->prev == NULL)
        goto unalloc_block;

    // If we are at head the next point needs to not be null
    // to avoid looking at restricted memory
    if(list_block == head && list_block->next != NULL && list_block->next->alloc)
        goto unalloc_block;

    // Same for tail
    if(list_block->next == NULL && list_block->prev != NULL && list_block->prev->alloc)
        goto unalloc_block;

    // In the middle 
    if(list_block->next != NULL && list_block->prev != NULL && 
        list_block->next->alloc && list_block->prev->alloc)
        goto unalloc_block;

    // TODO: Logic clean up
    memoryList *tmp = list_block;
    if(list_block->prev != NULL && !list_block->prev->alloc)
        merge_left(list_block);
    if(list_block->next != NULL && !list_block->next->alloc) {
        list_block->alloc = false;
        list_block = list_block->next;
        merge_left(list_block);
        free(list_block);
        return;
    }

    free(tmp);
    return;

unalloc_block:
    list_block->alloc = false;
}

memoryList *find_block(void* block)
{
    memoryList *current = head;
    while(current != NULL) {
        if(current->ptr == block)
            break;
        current = current->next;
    }

    return current;
}

void merge_left(memoryList *block)
{
    // Remove reference to the current block
    block->prev->next = block->next;

    if(block->next != NULL)
        block->next->prev = block->prev;

    // Merge sizes so the left side gets the total size
    block->prev->size += block->size;
    block->alloc = false;
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when refered to "memory" here, it is meant that the 
 * memory pool this module manages via initmem/mymalloc/myfree. 
 */

/* Get the number of contiguous areas of free space in memory. */
int mem_holes()
{
    int count = 0;
    memoryList *current = head;
    while(current != NULL) {
        if(!current->alloc)
            count++;
        current = current->next;
    }

    return count;
}

/* Get the number of bytes allocated */
int mem_allocated()
{
    int size = 0;
    memoryList *current = head;
    while(current != NULL) {
        if(current->alloc)
            size += current->size;
        current = current->next;
    }

    return size;
}

/* Number of non-allocated bytes */
int mem_free()
{
    return mem_total() - mem_allocated();
}

/* Number of bytes in the largest contiguous area of unallocated memory */
int mem_largest_free()
{
    int max = 0;
    memoryList *current = head;
    while(current != NULL)  {
        if(!current->alloc && current->size > max)
            max = current->size;
        current = current->next;
    }

    return max;
}

/* Number of free blocks smaller than or equal to "size" bytes. */
int mem_small_free(int size)
{
    int count = 0;
    memoryList *current = head;
    while(current != NULL) {
        if(!current->alloc && current->size <= size)
            count++;
        current = current->next;
    }

    return count;
}       

char mem_is_alloc(void *ptr)
{
    memoryList *current = head;
    while(current != NULL) {
        if(current->alloc && current->ptr == ptr)
            return '1';

        current = current->next; 
    }

    return '0';
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
    memoryList *current = head;
    while(current != NULL) {
        printf("Allocated: %s\tSize: %ld\tPtr: %p\n", current->alloc ? "true" : "false", current->size, current->ptr);
        current = current->next;
    }
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

    initmem(strat, 3);

    /* A simple example.  
       Each algorithm should produce a different layout. */

    a = mymalloc(1);
    b = mymalloc(1);
    c = mymalloc(1);

    myfree(b);
    myfree(c);
    myfree(a);

    print_memory();
    print_memory_status();

}
