#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "mymem.h"
#include "testrunner.h"

/* performs a randomized test:
	totalSize == the total size of the memory pool, as passed to initmem2
		totalSize must be less than 10,000 * minBlockSize
	fillRatio == when the allocated memory is >= fillRatio * totalSize, a block is freed;
		otherwise, a new block is allocated.
		If a block cannot be allocated, this is tallied and a random block is freed immediately thereafter in the next iteration
	minBlockSize, maxBlockSize == size for allocated blocks is picked uniformly at random between these two numbers, inclusive
	*/
void do_randomized_test(int strategyToUse, int totalSize, float fillRatio, int minBlockSize, int maxBlockSize, int iterations)
{
	void * pointers[10000];
	int storedPointers = 0;
	int strategy;
	int lbound = 1;
	int ubound = 4;
	int smallBlockSize = maxBlockSize/10;

	if (strategyToUse>0)
		lbound=ubound=strategyToUse;

	FILE *log;
	log = fopen("tests.log","a");
	if(log == NULL) {
	  perror("Can't append to log file.\n");
	  return;
	}

	fprintf(log,"Running randomized tests: pool size == %d, fill ratio == %f, block size is from %d to %d, %d iterations\n",totalSize,fillRatio,minBlockSize,maxBlockSize,iterations);

	fclose(log);

	for (strategy = lbound; strategy <= ubound; strategy++)
	{
		double sum_largest_free = 0;
		double sum_hole_size = 0;
		double sum_allocated = 0;
		int failed_allocations = 0;
		double sum_small = 0;
		struct timespec execstart, execend;
		int force_free = 0;
		int i;
		storedPointers = 0;

		initmem(strategy,totalSize);

		clock_gettime(CLOCK_REALTIME, &execstart);

		for (i = 0; i < iterations; i++)
		{
			if ( (i % 10000)==0 )
				srand ( time(NULL) );
			if (!force_free && (mem_free() > (totalSize * (1-fillRatio))))
			{
				int newBlockSize = (rand()%(maxBlockSize-minBlockSize+1))+minBlockSize;
				/* allocate */
				void * pointer = mymalloc(newBlockSize);
				if (pointer != NULL)
					pointers[storedPointers++] = pointer;
				else
				{ 
					failed_allocations++;
					force_free = 1;
				}
			}
			else
			{
				int chosen;
				void * pointer;

				/* free */
				force_free = 0;

				if (storedPointers == 0)
					continue;

				chosen = rand() % storedPointers;
				pointer = pointers[chosen];
				pointers[chosen] = pointers[storedPointers-1];

				storedPointers--;

				myfree(pointer);
			}

			sum_largest_free += mem_largest_free();
			sum_hole_size += (mem_free() / mem_holes());
			sum_allocated += mem_allocated();
			sum_small += mem_small_free(smallBlockSize);
		}

		clock_gettime(CLOCK_REALTIME, &execend);

		log = fopen("tests.log","a");
		if(log == NULL) {
		  perror("Can't append to log file.\n");
		  return;
		}
		
		fprintf(log,"\t=== %s ===\n",strategy_name(strategy));
		fprintf(log,"\tTest took %.2fms.\n", (execend.tv_sec - execstart.tv_sec) * 1000 + (execend.tv_nsec - execstart.tv_nsec) / 1000000.0);
		fprintf(log,"\tAverage hole size: %f\n",sum_hole_size/iterations);
		fprintf(log,"\tAverage largest free block: %f\n",sum_largest_free/iterations);
		fprintf(log,"\tAverage allocated bytes: %f\n",sum_allocated/iterations);
		fprintf(log,"\tAverage number of small blocks: %f\n",sum_small/iterations);
		fprintf(log,"\tFailed allocations: %d\n",failed_allocations);
		fclose(log);


	}
}

/* run randomized tests against the various strategies with various parameters */
int do_stress_tests(int argc, char **argv)
{
	int strategy = strategyFromString(*(argv+1));

	unlink("tests.log");  // We want a new log file

	do_randomized_test(strategy,10000,0.25,1,1000,10000);
	do_randomized_test(strategy,10000,0.25,1,2000,10000);
	do_randomized_test(strategy,10000,0.25,1000,2000,10000);
	do_randomized_test(strategy,10000,0.25,1,3000,10000);
	do_randomized_test(strategy,10000,0.25,1,4000,10000); 
	do_randomized_test(strategy,10000,0.25,1,5000,10000);

	do_randomized_test(strategy,10000,0.5,1,1000,10000);
	do_randomized_test(strategy,10000,0.5,1,2000,10000);
	do_randomized_test(strategy,10000,0.5,1000,2000,10000);
	do_randomized_test(strategy,10000,0.5,1,3000,10000); 
	do_randomized_test(strategy,10000,0.5,1,4000,10000);
	do_randomized_test(strategy,10000,0.5,1,5000,10000);

	do_randomized_test(strategy,10000,0.5,1000,1000,10000); /* watch what happens with this test!...why? */

	do_randomized_test(strategy,10000,0.75,1,1000,10000);
	do_randomized_test(strategy,10000,0.75,500,1000,10000);
	do_randomized_test(strategy,10000,0.75,1,2000,10000); 

	do_randomized_test(strategy,10000,0.9,1,500,10000); 

	return 0; /* you nominally pass for surviving without segfaulting */
}

/* basic sequential allocation of single byte blocks */
int test_alloc_1(int argc, char **argv) {
	strategies strategy;
	int lbound = 1;
	int ubound = 4;

	if (strategyFromString(*(argv+1))>0)
		lbound=ubound=strategyFromString(*(argv+1));

	for (strategy = lbound; strategy <= ubound; strategy++)
	{
		int correct_holes = 0;
		int correct_alloc = 100;
		int correct_largest_free = 0;
		int i;

		void* lastPointer = NULL;
		initmem(strategy,100);
		for (i = 0; i < 100; i++)
		{
			void* pointer = mymalloc(1);
			if ( i > 0 && pointer != (lastPointer+1) )
			{
				printf("Allocation with %s was not sequential at %i; expected %p, actual %p\n", strategy_name(strategy), i,lastPointer+1,pointer);
				return 1;
			}
			lastPointer = pointer;
		}

		if (mem_holes() != correct_holes)
		{
			printf("Holes not counted as %d with %s\n", correct_holes, strategy_name(strategy));
			return	1;
		}

		if (mem_allocated() != correct_alloc)
		{
			printf("Allocated memory not reported as %d with %s\n", correct_alloc, strategy_name(strategy));
			return	1;
		}

		if (mem_largest_free() != correct_largest_free)
		{
			printf("Largest memory block free not reported as %d with %s\n", correct_largest_free, strategy_name(strategy));
			return	1;
		}

	}

	return 0;
}


/* alloc, alloc, free, alloc */
int test_alloc_2(int argc, char **argv) {
	strategies strategy;
	int lbound = 1;
	int ubound = 4;

	if (strategyFromString(*(argv+1))>0)
		lbound=ubound=strategyFromString(*(argv+1));

	for (strategy = lbound; strategy <= ubound; strategy++)
	{
		int correct_holes;
		int correct_alloc;
		int correct_largest_free;
		int correct_small;
		void* first;
		void* second;
		void* third;
		int correctThird;

		initmem(strategy,100);

		first = mymalloc(10);
		second = mymalloc(1);
		myfree(first);
		third = mymalloc(1);

		if (second != (first+10))
		{
			printf("Second allocation failed; allocated at incorrect offset with strategy %s", strategy_name(strategy));
			return 1;
		}

		correct_alloc = 2;
		correct_small = (strategy == First || strategy == Best);

		switch (strategy)
		{
			case Best:
				correctThird = (third == first);
				correct_holes = 2;
				correct_largest_free = 89;
				break;
			case Worst:
				correctThird = (third == second+1);
				correct_holes = 2;
				correct_largest_free = 88;
				break;
			case First:
				correctThird = (third == first);
				correct_holes = 2;
				correct_largest_free = 89;
				break;
			case Next:
				correctThird = (third == second+1);
				correct_holes = 2;
				correct_largest_free = 88;
				break;
		        case NotSet:
			        break;
		}

		if (!correctThird)
		{
			printf("Third allocation failed; allocated at incorrect offset with %s", strategy_name(strategy));
			return 1;
		}

		if (mem_holes() != correct_holes)
		{
			printf("Holes counted as %d, should be %d with %s\n", mem_holes(), correct_holes, strategy_name(strategy));
			return	1;
		}

		if (mem_small_free(9) != correct_small)
		{
			printf("Small holes counted as %d, should be %d with %s\n", mem_small_free(9), correct_small, strategy_name(strategy));
			return	1;
		}

		if (mem_allocated() != correct_alloc)
		{
			printf("Memory reported as %d, should be %d with %s\n", mem_allocated(0), correct_alloc, strategy_name(strategy));
			return	1;
		}

		if (mem_largest_free() != correct_largest_free)
		{
			printf("Largest memory block free reported as %d, should be %d with %s\n", mem_largest_free(), correct_largest_free, strategy_name(strategy));
			return	1;
		}

	}

	return 0;
}


/* basic sequential allocation followed by 50 frees */
int test_alloc_3(int argc, char **argv) {
	strategies strategy;
	int lbound = 1;
	int ubound = 4;

	if (strategyFromString(*(argv+1))>0)
		lbound=ubound=strategyFromString(*(argv+1));

	for (strategy = lbound; strategy <= ubound; strategy++)
	{
		int correct_holes = 50;
		int correct_alloc = 50;
		int correct_largest_free = 1;
		int i;

		void* lastPointer = NULL;
		initmem(strategy,100);
		for (i = 0; i < 100; i++)
		{
			void* pointer = mymalloc(1);
			if ( i > 0 && pointer != (lastPointer+1) )
			{
				printf("Allocation with %s was not sequential at %i; expected %p, actual %p\n", strategy_name(strategy), i,lastPointer+1,pointer);
				return 1;
			}
			lastPointer = pointer;
		}

		for (i = 1; i < 100; i+= 2)
		{
			myfree(mem_pool() + i);
		}

		if (mem_holes() != correct_holes)
		{
			printf("Holes not counted as %d with %s\n", correct_holes, strategy_name(strategy));
			return	1;
		}

		if (mem_allocated() != correct_alloc)
		{
			printf("Memory not reported as %d with %s\n", correct_alloc, strategy_name(strategy));
			return	1;
		}

		if (mem_largest_free() != correct_largest_free)
		{
			printf("Largest memory block free not reported as %d with %s\n", correct_largest_free, strategy_name(strategy));
			return	1;
		}

		for(i=0;i<100;i++) {
		  if(mem_is_alloc(mem_pool()+i) == i%2) {
		    printf("Byte %d in memory claims to ",i);
		    if(i%2)
		      printf("not ");
		    printf("be allocated.  It should ");
		    if(!i%2)
		      printf("not ");
		    printf("be allocated.\n");
		    return 1;
		  }
		}
	}

	return 0;
}


/* basic sequential allocation followed by 50 frees, then another 50 allocs */
int test_alloc_4(int argc, char **argv) {
	strategies strategy;
	int lbound = 1;
	int ubound = 4;

	if (strategyFromString(*(argv+1))>0)
		lbound=ubound=strategyFromString(*(argv+1));

	for (strategy = lbound; strategy <= ubound; strategy++)
	{
		int correct_holes = 0;
		int correct_alloc = 100;
		int correct_largest_free = 0;
		int i;

		void* lastPointer = NULL;
		initmem(strategy,100);
		for (i = 0; i < 100; i++)
		{
			void* pointer = mymalloc(1);
			if ( i > 0 && pointer != (lastPointer+1) )
			{
				printf("Allocation with %s was not sequential at %i; expected %p, actual %p\n", strategy_name(strategy), i,lastPointer+1,pointer);
				return 1;
			}
			lastPointer = pointer;
		}

		for (i = 1; i < 100; i+= 2)
		{
			myfree(mem_pool() + i);
		}

		for (i = 1; i < 100; i+=2)
		{
			void* pointer = mymalloc(1);
			if ( i > 1 && pointer != (lastPointer+2) )
			{
				printf("Second allocation with %s was not sequential at %i; expected %p, actual %p\n", strategy_name(strategy), i,lastPointer+1,pointer);
				return 1;
			}
			lastPointer = pointer;
		}

		if (mem_holes() != correct_holes)
		{
			printf("Holes not counted as %d with %s\n", correct_holes, strategy_name(strategy));
			return	1;
		}

		if (mem_allocated() != correct_alloc)
		{
			printf("Memory not reported as %d with %s\n", correct_alloc, strategy_name(strategy));
			return	1;
		}

		if (mem_largest_free() != correct_largest_free)
		{
			printf("Largest memory block free not reported as %d with %s\n", correct_largest_free, strategy_name(strategy));
			return	1;
		}

	}

	return 0;
}


int run_memory_tests(int argc, char **argv)
{
	if (argc < 3)
	{
	        printf("Usage: mem -test <test> <strategy> \n");
		return 0;
	}
	set_testrunner_default_timeout(20);
	/* Tests can be invoked by matching their name or their suite name or 'all'*/
	testentry_t tests[] = {
		{"alloc1","suite1",test_alloc_1},
		{"alloc2","suite2",test_alloc_2},
		{"alloc3","suite1",test_alloc_3},
		{"alloc4","suite2",test_alloc_4},
		{"stress","suite3",do_stress_tests},
	};

 	return run_testrunner(argc,argv,tests,sizeof(tests)/sizeof(testentry_t));
}

int main(int argc, char **argv)
{
  if( argc < 2) {
    printf("Usage: mem -test <test> <strategy> | mem -try <arg1> <arg2> ... \n");
    exit(-1);
  }
  else if (!strcmp(argv[1],"-test"))
    return run_memory_tests(argc-1,argv+1);
  else if (!strcmp(argv[1],"-try")) {
    try_mymem(argc-1,argv+1);
    return 0;
  } else {
    printf("Usage: mem -test <test> <strategy> | mem -try <arg1> <arg2> ... \n");
    exit(-1);
  }

}
