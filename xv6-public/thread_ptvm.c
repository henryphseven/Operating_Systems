//Extra credit (10%): Cute macros for per-thread variables
#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "lock.h"

#define MAX_THREADS 10

DEFINE_PER_THREAD(balance_t, balance);

volatile int total_balance = 0;

struct thread_mutex lock; //lock for protecting total_balance
struct thread_mutex lock_1; //lock for protecting printf

void do_work(void *arg){

    int i;
    int old;

    int id = gettid();
    if(id < 0){

    	printf(1, "thread: error\n");
    	thread_exit();
        return;
    }
    char *name = per_thread(balance).name;
	int a = per_thread(balance).amount;

	thread_mutex_lock(&lock_1);
	printf(1, "[thread %d: %s] Start to do work. Workload: %d\n", id, name, a);
	thread_mutex_unlock(&lock_1);

    for (i = 0; i < a; i++) {

    	thread_mutex_lock(&lock);

    	if(i == a/2){

    		 thread_mutex_lock(&lock_1);
    		 printf(1, "[thread %d: %s] Working. Progress: %d\n", id, name, i);
    		 thread_mutex_unlock(&lock_1);
    	}

        old = total_balance;
        delay(100000);
        total_balance = old + 1;
        thread_mutex_unlock(&lock);
    }

	thread_mutex_lock(&lock_1);
	printf(1, "[thread %d: %s] Work done. Finished work: %d\n", id, name, a);
	thread_mutex_unlock(&lock_1);

    thread_exit();

    return;
}

int main(int argc, char *argv[]) {

  char *s[MAX_THREADS];
  int t[MAX_THREADS], r[MAX_THREADS];
  int correct_balance = 0;

  thread_mutex_init(&lock, "total_balance");
  thread_mutex_init(&lock_1, "printf");

  int i;

  //int thread_create(void(*fcn)(void*), void *arg, void*stack)
  for(i = 0; i < MAX_THREADS; i++){

	  balance[i].name[0] = 65+i;
	  balance[i].name[1] = '\0';
	  balance[i].amount = (i+1)*1000;
	  correct_balance = correct_balance + balance[i].amount;
	  //To make sure that stacks are page aligned, allocate them with sbrk().
	  s[i] = sbrk(PGSIZE);
	  t[i] = thread_create(do_work, (void*)&balance[i], (void*)s[i]);
  }

  //main thread collects exited child threads' pids
  for(i = 0; i < MAX_THREADS; i++){

	  r[i] = thread_join();
  }

  printf(1, "[main thread] Threads finished: ");

  for(i = 0; i < MAX_THREADS - 1; i++){

	  printf(1, "(%d):%d, ", t[i], r[i]);
  }
  printf(1, "(%d):%d\n", t[MAX_THREADS - 1], r[MAX_THREADS - 1]);

  printf(1, "[main thread] Shared balance: %d, correct balance: %d, diffs: %d\n", total_balance, correct_balance, total_balance - correct_balance);

  exit();
}
