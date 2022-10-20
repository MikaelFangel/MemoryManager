#include "mymem.h"

strategies myStrategy = NotSet;    // Current strategy

size_t mySize;
void *myMemory = NULL;

static memoryList *head;
static memoryList *last_allocated;

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
    // Setup variables and free if they have been used before.
    myStrategy = strategy;
    if (myMemory != NULL){
        free(myMemory);
        myMemory = NULL;
    }

    // Free all blocks if the structure is already instantiated
    memoryList *current = head;
    memoryList *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    // Setup memory block
    mySize = sz;
    myMemory = malloc(sz);

    // Setup linked list memory structure
    head = (memoryList *) malloc(sizeof(memoryList));
    head->alloc = false;
    head->size = mySize;
    head->ptr = myMemory;
    head->next = NULL;
    head->prev = NULL;
    last_allocated = head;
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1 
 */
void *mymalloc(size_t requested)
{
    assert((int) myStrategy > 0);
    // Check if there is a block large enough to hold the requested
    if (requested > mem_largest_free())
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
            return allocate_block_of_memory(nextfit(requested), requested);
        default:
            return NULL;
    }
}

void *allocate_block_of_memory(memoryList *block_to_allocate, size_t requested_size)
{
    assert(block_to_allocate->size >= requested_size);
    // Check the block recieved is a valid mem location
    if (block_to_allocate == NULL)
        return NULL;

    // If the position given is equal to the requested size
    // then overtake the block and return a pointer
    if (block_to_allocate->size == requested_size)
    {
        block_to_allocate->alloc = true;
        return block_to_allocate->ptr;
    }

    // Request block is smaller than the block to allocate and we therefore
    // need to divide the memory into two
    memoryList *split_block = (memoryList *) malloc(sizeof(memoryList));

    // Update head if we take its place with our split
    if (block_to_allocate == head)
        head = split_block;
    // If we're splitting then we should also move our last pointer
    last_allocated = split_block;

    // Setting values for the left side of our split block
    split_block->alloc = true;
    split_block->size = requested_size;
    split_block->ptr = block_to_allocate->ptr;
    split_block->next = block_to_allocate;
    split_block->prev = block_to_allocate->prev;

    // Setting value for the right side of our split
    block_to_allocate->size -= requested_size;
    block_to_allocate->ptr = block_to_allocate->ptr + requested_size;
    // Make sure to keep the linked list connected
    if (block_to_allocate->prev != NULL)
        block_to_allocate->prev->next = split_block;
    block_to_allocate->prev = split_block;

    return split_block->ptr;
}

/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void* block)
{
    // Setup shorthands
    memoryList *block_to_unalloc = find_block(block);
    memoryList *nextBlock = block_to_unalloc->next;
    memoryList *prevBlock = block_to_unalloc->next;

    if (block_to_unalloc == NULL)
        return;

    // Freeing the only block in the memory list
    if (nextBlock == NULL && prevBlock == NULL)
        goto unalloc_block;

    // If we are at head the next point needs to not be null
    // to avoid looking at restricted memory
    if (block_to_unalloc == head && nextBlock != NULL && nextBlock->alloc)
        goto unalloc_block;

    // Same for tail
    if (nextBlock == NULL && prevBlock != NULL && prevBlock->alloc)
        goto unalloc_block;

    // In the middle 
    if (nextBlock != NULL && prevBlock != NULL &&
       nextBlock->alloc && prevBlock->alloc)
        goto unalloc_block;

    // TODO: Logic clean up
    memoryList *tmp = block_to_unalloc;
    if (prevBlock != NULL && !prevBlock->alloc)
        merge_left(block_to_unalloc);
    if (nextBlock != NULL && !nextBlock->alloc)
    {
        block_to_unalloc->alloc = false;
        block_to_unalloc = nextBlock;
        merge_left(block_to_unalloc);
        free(block_to_unalloc);
        return;
    }

    free(tmp);
    return;

unalloc_block:
    block_to_unalloc->alloc = false;
}

memoryList *find_block(void* block)
{
    memoryList *current = head;
    while (current != NULL)
    {
        if (current->ptr == block)
            break;
        current = current->next;
    }
    return current;
}

void merge_left(memoryList *block)
{
    // Setup shorthands
    memoryList *nextBlock = block->next;
    memoryList *prevBlock = block->prev;

    // Ensure to move pointer for last allocated block, when merging
    if (block == last_allocated)
        last_allocated = prevBlock;

    // Remove reference to the current block
    prevBlock->next = nextBlock;

    if (nextBlock != NULL)
        nextBlock->prev = prevBlock;

    // Merge sizes so the left side gets the total size
    prevBlock->size += block->size;
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
    while (current != NULL)
    {
        if (!current->alloc)
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
    while (current != NULL)
    {
        if (current->alloc)
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
    while (current != NULL)
    {
        if (!current->alloc && current->size > max)
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
    while (current != NULL)
    {
        if (!current->alloc && current->size <= size)
            count++;
        current = current->next;
    }
    return count;
}       

char mem_is_alloc(void *ptr)
{
    memoryList *current = head;
    while (current != NULL)
    {
        if (current->alloc && current->ptr == ptr)
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

/**
 * Search the linked list from head until an unallocated block is found that is larger than or equal to the requsted size
 * @param requested size
 * @return memoryList ptr to the block of unallocated memory. Returns NULL if no block is found.
 */
memoryList *firstfit(size_t requested) {
    memoryList *current = head;
    while (current != NULL)
    {
        if (!current->alloc && current->size >= requested)
            break;
        current = current->next;
    }

    return current;
}

memoryList *worstfit(size_t requested)
{
    memoryList *current, *max_ptr;
    max_ptr = head;

    // Find the first unallocated struct
    while (max_ptr != NULL && max_ptr->alloc)
        max_ptr = max_ptr->next;

    // Return NULL if we couldn't find a free space
    if (max_ptr == NULL)
        return max_ptr;

    current = max_ptr;

    // Find the maximum sized free block
    while (current != NULL)
    {
        if (current->size > max_ptr->size && !current->alloc)
            max_ptr = current;
        current = current->next;
    }

    // Check if the requested size fit in the maximum sized block
    if (max_ptr->size >= requested)
        return  max_ptr;
    else
        return NULL;
}

memoryList *bestfit(size_t requested)
{
    memoryList *current = head;
    memoryList *bestfit = NULL;
    int smallest_diff = -1; // Start value

    while (current != NULL)
    {
        if (!current->alloc && current->size >= requested)
        {
            int diff = current->size - requested;

            // Set smallest diff
            if (smallest_diff == -1 || diff < smallest_diff)
            {
                smallest_diff = diff;
                bestfit = current;
            }
        }
        current = current->next;
    }

    // Return null if there is nothing found
    if (smallest_diff == -1)
        return NULL;

    return bestfit;
}

memoryList *nextfit(size_t requested)
{
    memoryList *current = last_allocated;
    if (current->next == NULL)
        current = head;
    else
        current = last_allocated->next;

    do
    {
        if (!current->alloc && current->size >= requested)
        {
            last_allocated = current;
            return current;
        }
        if (current->next == NULL)
            current = head;
        else
            current = current->next;
    }
    while (current != last_allocated->next);

    return NULL;
}


/* 
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory()
{
    memoryList *current = head;
    while (current != NULL)
    {
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
    if (argc > 1)
        strat = strategyFromString(argv[1]);
    else
        strat = Next;

    initmem(strat,100);

    a = mymalloc(10);
	b = mymalloc(1);
	myfree(a);
	c = mymalloc(1);


    printf("b + 1: %p\n", b+1);
    printf("c: %p\n", c);

    print_memory();
    print_memory_status();

}
