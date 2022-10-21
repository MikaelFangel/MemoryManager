#include "mymem.h"

strategies myStrategy = NotSet;    // Current strategy

size_t mySize;
void *myMemory = NULL;

static memoryList *head;
static memoryList *last_allocated;

/**
 * Initializes the memory and if called more than once it free the previous allocated memory
 * @param strategy can be either "first", "next", "worst" or "best"
 * @param sz how many bytes should be available for all malloc requests
 */
void initmem(strategies strategy, size_t sz)
{
    myStrategy = strategy;

    // If not the first time initmem is called then we free the old myMemory
    if (myMemory)
    {
        free(myMemory);
        myMemory = NULL;
    }

    // If current is not null then it's not the first time, and we free the old allocations
    memoryList *current = head;
    memoryList *next;
    while (current)
    {
        next = current->next;
        free(current);
        current = next;
    }

    // Allocate an actual block of memory to be used by the memory manager
    mySize = sz;
    myMemory = malloc(sz);

    // Initialize the data structure for the memory list
    head = (memoryList *) malloc(sizeof(memoryList));
    head->alloc = false;
    head->size = mySize;
    head->ptr = myMemory;
    head->next = NULL;
    head->prev = NULL;
    last_allocated = head;
}

/**
 *  Allocate a block of memory with the requested size.
 *  Restriction: requested >= 0
 * @param requested the size need for the block that should be allocated
 * @return the placement of the ptr in myMemory and NULL if no block was allocated
 */
void *mymalloc(size_t requested)
{
    assert((int)myStrategy > 0);
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

/**
 * Allocated the actual block in the memory list and move all pointer to keep the list linked
 * if the block to allocate size is the same at the requested size it overtakes the old block
 * else it while split the unallocated block to the left and take residence in the left side
 * this function have a side effect that updates the head due to the left splitting
 * @param block_to_allocate the block that should be allocated found with a strategy
 * @param requested_size the size to allocated the new block
 * @return the pointer to the myMemory location of the new block
 */
void *allocate_block_of_memory(memoryList *block_to_allocate, size_t requested_size)
{
    // Check the block received is a valid mem
    assert(block_to_allocate->size >= requested_size);
    if (!block_to_allocate)
        return NULL;

    // If the position given is equal to the requested size then overtake the block and return the pointer
    if (block_to_allocate->size == requested_size)
    {
        block_to_allocate->alloc = true;
        return block_to_allocate->ptr;
    }

    // Request block is smaller than the block to allocate, and we therefore need to divide the memory into two
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
    if (block_to_allocate->prev)
        block_to_allocate->prev->next = split_block;
    block_to_allocate->prev = split_block;

    return split_block->ptr;
}

/**
 * Search the linked list from head until an unallocated block is found that is larger than or
 * equal to the requested size
 * @param requested size
 * @return memoryList ptr to the block of unallocated memory. Returns NULL if no block is found.
 */
memoryList *firstfit(size_t requested) {
    memoryList *current;
    for (current = head; current; current = current->next)
        if (!current->alloc && current->size >= requested)
            break;

    return current;
}

/**
 * Search the memory list from head to the end and find the largest block free which
 * is larger than or equal to the requested size.
 * @param requested size of the block needed
 * @return memory list pointer to the free block and null if no free block available
 */
memoryList *worstfit(size_t requested)
{
    memoryList *current, *max_ptr;
    max_ptr = head;

    // Find the first unallocated struct
    while (max_ptr && max_ptr->alloc)
        max_ptr = max_ptr->next;

    // Return NULL if we couldn't find a free space
    if (!max_ptr)
        return max_ptr;

    // Find the maximum sized free block
    for (current = max_ptr; current; current = current->next)
        if (current->size > max_ptr->size && !current->alloc)
            max_ptr = current;

    // Check if the requested size fit in the maximum sized block
    if (max_ptr->size >= requested)
        return  max_ptr;
    else
        return NULL;
}

/**
 * Find the block with the best fit (the smallest difference in size) and return it
 * @param requested size of the block needed
 * @return memory list pointer to the free block and null if no free block available
 */
memoryList *bestfit(size_t requested)
{
    memoryList *bestfit = NULL;
    int smallest_diff = -1; // Start value

    for (memoryList *current = head; current; current = current->next)
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
    }

    // Return null if there is nothing found
    if (smallest_diff == -1)
        return NULL;

    return bestfit;
}

/**
 * Finds the first free block which fit the requested size from the location of the last allocated
 * @param requested size of the block needed
 * @return memory list pointer to the free block and null if no free block available
 */
memoryList *nextfit(size_t requested)
{
    memoryList *current = last_allocated;
    if (!current->next)
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
        if (!current->next)
            current = head;
        else
            current = current->next;
    }
    while (current != last_allocated->next);

    return NULL;
}

/**
 * Frees a block of memory previously allocated by mymalloc by find the memory list block that it
 * corresponds to and then either freeing it or setting its value to unallocated
 * @param block the block in myMemory to free
 */
void myfree(void* block)
{
    memoryList *block_to_unalloc = find_block(block);
    if (!block_to_unalloc)
        return;

    // Freeing the only block in the memory list
    if (!block_to_unalloc->next && !block_to_unalloc->prev)
        goto unalloc_block;

    // If we are at head the next point needs to not be null to avoid looking at restricted memory
    if (block_to_unalloc == head && block_to_unalloc->next && block_to_unalloc->next->alloc)
        goto unalloc_block;

    // Same for tail
    if (!block_to_unalloc->next && block_to_unalloc->prev && block_to_unalloc->prev->alloc)
        goto unalloc_block;

    // In the middle if both sides of the block is allocated
    if (block_to_unalloc->next && block_to_unalloc->prev &&
       block_to_unalloc->next->alloc && block_to_unalloc->prev->alloc)
        goto unalloc_block;


    memoryList *mergedBlock = block_to_unalloc;
    // Try to merge to the left
    if (block_to_unalloc->prev && !block_to_unalloc->prev->alloc){
         mergedBlock = merge_left(block_to_unalloc);
    }

    // Try to go right and merge left again
    if (mergedBlock->next && !mergedBlock->next->alloc){
        merge_left(mergedBlock->next);
    }
    return;

    unalloc_block:
        block_to_unalloc->alloc = false;
}

/**
 * Finds the corresponding memory list pointer to a given block
 * @param block block in myMemory to find
 * @return the memory list pointer to that block
 */
memoryList *find_block(void* block)
{
    memoryList *current;
    for(current = head; current; current = current->next)
        if (current->ptr == block)
            break;

    return current;
}

/**
 * Merge two unallocated block but always to the left. Which means the block given should always be right of
 * the block you want to merge with
 * @param block_to_unalloc block right of the one you want to merge with
 * @return the left side of the merge
 */
memoryList *merge_left(memoryList *block_to_unalloc)
{
    // Update pointer to last allocated block if the old one gets merged.
    if (block_to_unalloc == last_allocated)
        last_allocated = block_to_unalloc->prev;

    // Remove reference to the current block
    block_to_unalloc->prev->next = block_to_unalloc->next;
    if (block_to_unalloc->next)
        block_to_unalloc->next->prev = block_to_unalloc->prev;

    // Merge sizes so the left side gets the total size
    block_to_unalloc->prev->size += block_to_unalloc->size;
    block_to_unalloc->prev->alloc = false;

    memoryList *mergedBlock = block_to_unalloc->prev;

    free(block_to_unalloc);

    return mergedBlock;
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when referred to "memory" here, it is meant that the
 * memory pool this module manages via initmem/mymalloc/myfree.
 */

/* Get the number of contiguous areas of free space in memory. */
int mem_holes()
{
    int count = 0;
    for (memoryList *current = head; current; current = current->next)
        if (!current->alloc)
            count++;

    return count;
}

/* Get the number of bytes allocated */
int mem_allocated()
{
    size_t size = 0;
    for (memoryList *current = head; current; current = current->next)
        if (current->alloc)
            size += current->size;

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
    size_t max = 0;
    for (memoryList *current = head; current; current = current->next)
        if (!current->alloc && current->size > max)
            max = current->size;

    return max;
}

/* Number of free blocks smaller than or equal to "size" bytes. */
int mem_small_free(int size)
{
    int count = 0;
    for (memoryList *current = head; current; current = current->next)
        if (!current->alloc && current->size <= size)
            count++;

    return count;
}

char mem_is_alloc(void *ptr)
{
    for(memoryList *current = head; current; current = current->next)
        if (current->alloc && current->ptr == ptr)
            return '1';

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
    while (current)
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
