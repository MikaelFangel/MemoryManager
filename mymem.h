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

char *strategy_name(strategies);
strategies strategyFromString(char *);


void initmem(strategies, size_t);
void *mymalloc(size_t);
void myfree(void *);

int mem_holes(void);
int mem_allocated(void);
int mem_free(void);
int mem_total(void);
int mem_largest_free(void);
int mem_small_free(int);
char mem_is_alloc(void *);
void *mem_pool(void);
void print_memory(void);
void print_memory_status(void);
void try_mymem(int, char **);

void *allocate_block_of_memory(memoryList *, size_t);
memoryList *merge_left(memoryList *);
memoryList *find_block(void *);
memoryList *firstfit(size_t);
memoryList *worstfit(size_t);
memoryList *bestfit(size_t);
memoryList *nextfit(size_t);
