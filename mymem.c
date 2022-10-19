#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>


/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

strategies myStrategy = NotSet;    // Current strategy

size_t mySize;
void *myMemory = NULL;

static struct memoryList *head;
static struct memoryList *tail;
static struct memoryList *last; // Used in next-fit


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

void initmem(strategies strategy, size_t sz) {
    myStrategy = strategy;

    /* all implementations will need an actual block of memory to use */
    mySize = sz;

    if (myMemory != NULL) free(myMemory); /* in case this is not the first time initmem2 is called */

    /* TODO: release any other memory you were using for bookkeeping when doing a re-initialization! */
    struct memoryList *current = head;
    struct memoryList *next;
    if(head != NULL) {
        do {
            next = current->next;
            free(current);
            current = next;
        } while (current->next != head);

        free(current);
        current = NULL;
        next = NULL;
        head = NULL;
    }

    myMemory = malloc(sz);

    /* TODO: Initialize memory management structure. */
    // Allocate the size of memoryList to head.
    head = (struct memoryList *) malloc(sizeof(struct memoryList));
    head->alloc = '0';       // Not allocated
    head->size = (int)sz;    // Size of memory block
    head->ptr = myMemory;    // Point to the allocated memory block address
    // Circular linked list
    head->next = head;
    head->prev = head;
    tail = head;

    if(strategy == Next) {
        last = head;
    }
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1 
 */

void *mymalloc(size_t requested) {
    assert((int) myStrategy > 0);

    struct memoryList *memoryBlock;

    switch (myStrategy) {
        case NotSet:
            return NULL;
        case First:
            memoryBlock = firstfit(requested);
            break;
        case Best:
            memoryBlock = bestfit(requested);
            break;
        case Worst:
            memoryBlock = worstfit(requested);
            break;
        case Next:
            memoryBlock = nextfit(requested);
    }
    // If no memory block fits requested size
    if (memoryBlock == NULL)
        return NULL;

    return insertNode(memoryBlock, requested);
}

/**
 * Insert a node memory block in the linked list
 * @param block the block of free memory to insert new block in
 * @param requested the requested size of the new block
 * @return pointer to the newly inserted block
 */
void *insertNode(struct memoryList *block, size_t requested) {
    // blockToInsert takes up space in "front" of the block.
    // Allocate space for the new block node
    struct memoryList *blockToInsert = (struct memoryList *) malloc(sizeof(struct memoryList));
    blockToInsert->alloc = '1';
    blockToInsert->size = (int) requested;
    blockToInsert->ptr = block->ptr;

    // Make the block smaller by the size of blockToInsert and adjust pointers.
    block->size = block->size - requested;
    block->ptr = block->ptr + requested;

    // Check if block is head and adjust head to the new node
    if (head->ptr == block->ptr) {
        head = blockToInsert;
    }

    // Insert node and adjust pointers
    block->prev->next = blockToInsert;
    blockToInsert->prev = block->prev;
    blockToInsert->next = block;
    block->prev = blockToInsert;

    if(block->size == 0) {
        myfree(block->ptr);
    }
    last = blockToInsert;

    // Return pointer to allocated block
    return blockToInsert->ptr;
}

/**
 * Search the linked list from head until an unallocated block is found that is larger than or equal to the requsted size
 * @param requested size
 * @return memoryList ptr to the block of unallocated memory. Returns NULL if no block is found.
 */
struct memoryList *firstfit(size_t requested) {
    struct memoryList *current = head;
    do {
        if ((current->alloc == '0') && (current->size >= requested))
            return current;
        current = current->next;
    } while (current != head);
    return NULL;
}

struct memoryList *worstfit(size_t requested) {
    struct memoryList *current, *max_ptr;
    max_ptr = head;

    // Find the first unallocated struct
    if(head->alloc == '1') {
        do {
            max_ptr = max_ptr->next;
        } while(max_ptr->alloc == '1' && max_ptr != head);
    }

    // Return NULL if we couldn't find a free space
    if(max_ptr->alloc == '1')
        return NULL;

    current = max_ptr;
    // Find the maximum sized free block
    do {
        if (current->size > max_ptr->size && current->alloc == '0') {
            max_ptr = current;
        }

        current = current->next;
    } while (current != head);

    // Check if the requested size fit in the maximum sized block
    if(max_ptr->size >= requested)
        return max_ptr;
    else
        return NULL;
}

/**
 * TODO: Find a better solution than to search from tail - only works in the beginning. How to keep track of last allocated block?
 * Search the linked list from tail until an unallocated block is found that is larger than or equal to the requsted size
 * @param requested size
 * @return memoryList ptr to the block of unallocated memory. Returns NULL if no block is found.
 */
struct memoryList *nextfit(size_t requested) {
    struct memoryList *current = last->next;
    do {
        if ((current->alloc == '0') && (current->size >= requested))
            return current;
        current = current->next;
    } while (current != last);
}

struct memoryList *bestfit(size_t requested) {
    struct memoryList *current = head;
    struct memoryList *bestfit = NULL;
    int smallestDiff = -1; // Start value

    // Loop
    do {
        if ((current->alloc == '0') && (current->size >= requested)) {
            int diff = current->size - requested;

            // Set smallest diff
            if (smallestDiff == -1) {
                smallestDiff = diff;
                bestfit = current;
            }
            else if (diff < smallestDiff) {
                smallestDiff = diff;
                bestfit = current;
            }
        }
        current = current->next;
    } while (current != head);

    // Return null if there is nothing found
    if (smallestDiff == -1) return NULL;
    return bestfit;
}

/**
 * Frees a block of memory previously allocated by mymalloc
 * @param block pointer to the block to be freed
 */
void myfree(void *block) {
    // Search linked list until block is found
    struct memoryList *current = head;
    do {
        if (current->ptr == block)
            break;
        current = current->next;
    } while (current != head);

    // Mark block unallocated
    current->alloc = '0';

    // Check if previous is unallocated, merge with current
    if(current->prev->alloc == '0' && current != head) {
        // Link previous to next and vice versa
        current->prev->next = current->next;
        current->next->prev = current->prev;
        // Merge the size of previous block with current
        current->prev->size += current->size;

        // Check if merging with head and adjust head
        if(current->prev == head) {
            head = current;
        }
        free(current->prev);
    }

    // Check if next is unallocated, merge with current
    if(current->next->alloc == '0' && current->alloc == '0' && current != tail) {
        // Link next next to current and vice versa
        current->next->next->prev = current;
        current->next = current->next->next;
        // Merge the size of current with the next block
        current->size += current->next->size;

        // If current->next is head, make current head and free next
        if(current->next == head) {
            head = current;
            free(current->next);
        } else {
            free(current->next);
        }
    }

    // Free if memory block is empty
    if(current->size == 0) {
        // Link prev and next block
        current->prev->next = current->next;
        current->next->prev = current->prev;
        // Check for head and tail and adjust
        if(current == head) {
            head = current->next;
        }
        if(current == tail) {
            tail = current->prev;
        }
        free(current);
    }
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
    do {
        if (current->alloc == '0') {
            count++;
        }
        current = current->next;
    } while (current != head);
    return count;
}

/* Get the number of bytes allocated */
// Might have misunderstood, but this is calculating allocated size and not the number of allocated blocks as guide suggests
int mem_allocated() {
    int size = 0;
    struct memoryList *current = head;
    do {
        if (current->alloc == '1') {
            size += current->size;
        }
        current = current->next;
    } while (current != head);
    return size;
}

/* Number of non-allocated bytes */
int mem_free() {
    return mem_total() - mem_allocated();
}

/* Number of bytes in the largest contiguous area of unallocated memory */
// Iterate list and track of largest unallocated block
int mem_largest_free() {
    int max = 0;
    struct memoryList *current = head;
    do {
        if ((current->alloc == '0') && (current->size > max))
            max = current->size;
        current = current->next;
    } while (current != head);
    return max;
}

/* Number of free blocks smaller than or equal to "size" bytes. */
// Iterate list and increment counter when block size < size
int mem_small_free(int size) {
    int count = 0;
    struct memoryList *current = head;
    do {
        if ((current->alloc == '0') && (current->size <= size))
            count++;
        current = current->next;
    } while (current != head);
    return count;
}

// Is a byte allocated?
char mem_is_alloc(void *ptr) {
    struct memoryList *current = head;
    do {
        if(current->ptr == ptr)
            return '1';
        current = current->next;
    } while (current != head);
    return '0';
}

/* 
 * Feel free to use these functions, but do not modify them.  
 * The test code uses them, but you may find them useful.
 */


//Returns a pointer to the memory pool.
void *mem_pool() {
    return myMemory;
}

// Returns the total number of bytes in the memory pool. */
int mem_total() {
    return mySize;
}


// Get string name for a strategy. 
char *strategy_name(strategies strategy) {
    switch (strategy) {
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
strategies strategyFromString(char *strategy) {
    if (!strcmp(strategy, "best")) {
        return Best;
    } else if (!strcmp(strategy, "worst")) {
        return Worst;
    } else if (!strcmp(strategy, "first")) {
        return First;
    } else if (!strcmp(strategy, "next")) {
        return Next;
    } else {
        return 0;
    }
}


/* 
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory() {
    struct memoryList *current = head;
    do {
        printf("Allocated: %c\tSize: %d\tPtr: %p\n", current->alloc, current->size, current->ptr);
        current = current->next;
    } while (current != head);
}

/* Use this function to track memory allocation performance.  
 * This function does not depend on your implementation, 
 * but on the functions you wrote above.
 */
void print_memory_status() {
    printf("%d out of %d bytes allocated.\n", mem_allocated(), mem_total());
    printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n", mem_free(), mem_holes(),
           mem_largest_free());
    printf("Average hole size is %f.\n\n", ((float) mem_free()) / mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char **argv) {
    strategies strat;
    void *a, *b, *c, *d, *e;
    if (argc > 1)
        strat = strategyFromString(argv[1]);
    else
        strat = First;


    /* A simple example.
       Each algorithm should produce a different layout. */

    initmem(strat, 6);

    a = mymalloc(2);
    b = mymalloc(2);
    c = mymalloc(2);
    myfree(b);

    print_memory();
    print_memory_status();

}
