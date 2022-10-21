#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

typedef enum strategies_enum
{
	NotSet = 0,
	Best = 1,
	Worst = 2,
	First = 3,
	Next = 4
} strategies;

typedef struct memoryList
{
    // doubly-linked list
    struct memoryList *prev;
    struct memoryList *next;

    size_t size;
    bool alloc;

    void *ptr;
} memoryList;

char *strategy_name(strategies strategy);
strategies strategyFromString(char * strategy);


void initmem(strategies strategy, size_t sz);
void *mymalloc(size_t requested);
void myfree(void* block);

int mem_holes(void);
int mem_allocated(void);
int mem_free(void);
int mem_total(void);
int mem_largest_free(void);
int mem_small_free(int size);
char mem_is_alloc(void *ptr);
void* mem_pool(void);
void print_memory(void);
void print_memory_status(void);
void try_mymem(int argc, char **argv);

void *allocate_block_of_memory(memoryList *block_to_allocate, size_t requested_size);
memoryList *merge_left(memoryList *block);
memoryList *find_block(void* block);
memoryList *firstfit(size_t requested);
memoryList *worstfit(size_t requested);
memoryList *bestfit(size_t requested);
memoryList *nextfit(size_t requested);