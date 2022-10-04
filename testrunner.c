/*
A simple testrunner framework
Original Author: L. Angrave
*/
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "testrunner.h"
#include "mymem.h"

/* Constants */
#define false (0)
#define true (1)
#define test_killed (2)
/* defaults */
static int default_timeout_seconds=5;
static int timeout_seconds;

void set_testrunner_default_timeout(int s) {
	assert(s>0);
	default_timeout_seconds=s;
}

void set_testrunner_timeout(int s) {
	assert(s>0);
	timeout_seconds=s;
}

/*  --- Helper macros and functions  --- */
#define DIE(mesg) {fprintf(stderr,"\n%s(%d):%s\n",__fname__,__LINE__,mesg); exit(1);}
static int eql( char*s1, char*s2) {return s1&&s2&&!strcmp(s1,s2);}

/* Callback function for qsort on strings */
static int mystrcmp( const void *p1, const void *p2) {
	return eql( ( char*)p1, ( char*)p2);
}

/* Stats of all tests run so far */
typedef struct
{
  int ran, passed, failed;
} stats_t;

/* -- Signal handlers -- */
static pid_t child_pid;
static int sent_child_timeout_kill_signal;

static void kill_child_signal_handler(intsigno) {
	if(!child_pid) return;
	char m[]="-Timeout(Killing test process)-";
	write(0,m,sizeof(m)-1);
	kill(child_pid,SIGKILL);
	sent_child_timeout_kill_signal=1;
}


/* Internal function to run a test as a forked child. The child process is terminated if it runs for more than a few seconds */

static int invoke_test_with_timelimit(testentry_t* test, int redirect_stdouterr,int argc, char **argv)
{
	char fname[255];
	int wait_status;
	pid_t wait_val;
	struct sigaction action;


	assert(!child_pid);
	assert(test && test->test_function && test->name);

	set_testrunner_timeout(default_timeout_seconds);

	errno=0;
    child_pid = fork ();
      if (child_pid == -1) {
		fprintf(stderr,"-fork failed so running test inline-");
		return test->test_function (argc, argv);
	}


    if (child_pid == 0)
	{
		if(redirect_stdouterr) {
			snprintf(fname,(int)sizeof(fname),"stdout-%s.txt",test->name);
			fname[sizeof(fname)-1]=0;
			freopen(fname, "w", stdout);
			memcpy(fname+3,"err",3);
			freopen(fname, "w", stderr);
		}
	  	exit(test->test_function(argc,argv));
	}else {

		wait_status=-1;
		sigemptyset(&action.sa_mask);

		action.sa_handler=kill_child_signal_handler;
		sigaction(SIGALRM,&action,NULL);
		sent_child_timeout_kill_signal=0;
		alarm(timeout_seconds);

		wait_val = waitpid (child_pid, &wait_status, 0);
		int child_exited_normally= WIFEXITED (wait_status);
		int child_exit_value=WEXITSTATUS (wait_status);
		int child_term_by_signal=WIFSIGNALED(wait_status);
		int child_term_signal=WTERMSIG(wait_status);

		if(child_term_by_signal) {
			fprintf(stderr,"testrunner:Test terminated by signal %d\n",child_term_signal);
			fprintf(stderr,"testrunner:waitpid returned %d (child_pid=%d,wait_status=%d)",wait_val,child_pid,wait_status);
		}

		if(child_pid != wait_val)
			fprintf(stderr,"testrunner: strange... wait_val != child_pid\n");

		int passed= (child_pid == wait_val) && (child_exit_value==0) && (child_exited_normally!=0);

		alarm(0);
		kill(child_pid,SIGKILL);
		child_pid=0;

		return sent_child_timeout_kill_signal ? test_killed :  passed ? 0 : 1;
	}
}


  /*
   * run a test and update the stats. The main guts of this functionality is provided by invoke_test_with_timelimit
   * This outer wrapper updates thes output and statistics before and after running the test.
   */
static int
run_one_test (stats_t * stats, testentry_t * test, int redirect_stdouterr,int argc, char **argv)
{
  int test_result;


  assert (stats && test->name && argc > 0 && argv && *argv);
  stats->ran++;
  stats->failed++;

  printf ("%2d.%-20s:", stats->ran, test->name);

  fflush(stdout);
	test_result=invoke_test_with_timelimit(test,redirect_stdouterr,argc,argv);


if (test_result == 0)
    {
      stats->failed--;
      stats->passed++;
    }
  printf(":%s\n", (test_result == 0 ? "pass" : test_result ==
	   2 ? "TIMEOUT * " : "FAIL *"));
	return test_result!=0;
}

/* Help functionality to print out sorted list of test names and suite names */
static void print_targets(testentry_t tests[], int count) {
	 char**array;
	char *previous;
	int i;

	array=(char**)calloc(sizeof(char*),count);

	/* Sort the test names and print unique entries*/

	for(i=0;i<count;i++) array[i]=tests[i].name;
	qsort(array,count,sizeof(array[0]),mystrcmp);

	printf("\nValid tests : all");
	for(i=0,previous="";i<count; i++) if(!eql(previous,array[i])) printf(" %s",(previous=array[i]));

	/* Sort the suite names and print unique entries*/
	for(i=0;i<count;i++) array[i]=tests[i].suite;
	qsort(array, count,sizeof(array[0]),mystrcmp);

	printf("\nValid suites:");

	for(i=0,previous="";i<count; i++) if(!eql(previous,array[i])) printf(" %s",(previous=array[i]));
	printf("\nValid strategies: all ");

	for(i=1;i<5;i++)
	  printf("%s ",strategy_name(i));
	printf("\n");

}



  /*
   * Main entry point for test harness
   */
int
run_testrunner(int argc, char **argv,testentry_t tests[],int test_count)
{
	char *test_name, *target;
	int i;
	stats_t stats;
	int target_matched,max_errors_before_quit,redirect_stdouterr;
	memset (&stats, 0, sizeof (stats));

	max_errors_before_quit=1;
	redirect_stdouterr=0;

	assert (tests != NULL);
	assert(test_count>0);
	assert (argc > 0 && argv && *argv);
	while(true) {
	target = argc > 1 ? argv[1] : "";
	assert (target);
	if(*target!='-') break;
	argc--;argv++;
	if(target[1]=='f' && target[2])
		max_errors_before_quit=atoi(target+1);
	else if(target[1]=='r')
		redirect_stdouterr=1;
	}

	target_matched = false;

	for (i=0;i<test_count && (max_errors_before_quit<1 || stats.failed != max_errors_before_quit);i++) {
	  test_name = tests[i].name;

	  assert(test_name);
	  assert(tests[i].suite);
	  assert(tests[i].test_function);
	  if (eql(target,test_name)||eql(target,"all") || eql (target,tests[i].suite) ) {
		if(!target_matched) printf("Running tests...\n");
	  target_matched = true;
	  run_one_test (&stats, &tests[i],redirect_stdouterr, argc - 1,argv + 1);
	}
	}
	if (!target_matched)
	{
	  fprintf (stderr, "Test '%s' not found", (strlen(target)>0?target : "(empty)"));
	print_targets(tests,test_count);
	}
	else {
	  printf ("\nTest Results:%d tests,%d passed,%d failed.\n", stats.ran,
	  stats.passed, stats.failed);
	}

	return stats.passed == stats.ran && target_matched ? 0 : 1;

}
