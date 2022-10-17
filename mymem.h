#include <stddef.h>

typedef enum strategies_enum {
	NotSet = 0,
	Best = 1,
	Worst = 2,
	First = 3,
	Next = 4
} strategies;

struct memoryList {
    // doubly-linked list
    struct memoryList *prev;
    struct memoryList *next;

    int size;            // How many bytes in this block?
    char alloc;          // 1 if this block is allocated,
    // 0 if this block is free.
    void *ptr;           // location of block in memory pool.
};

char *strategy_name(strategies strategy);
strategies strategyFromString(char * strategy);


void initmem(strategies strategy, size_t sz);
void *mymalloc(size_t requested);

void* insertNode(struct memoryList *block, size_t size);

struct memoryList *firstfit(size_t requested);
void myfree(void* block);

int mem_holes(  );
int mem_allocated();
int mem_free();
int mem_total();
int mem_largest_free();
int mem_small_free(int size);
char mem_is_alloc(void *ptr);
void* mem_pool();
void print_memory();
void print_memory_status();
void try_mymem(int argc, char **argv);
