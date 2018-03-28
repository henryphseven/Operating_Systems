//Extra credit (15%): Conditional variables
#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "lock.h"

#define THREAD_NUM 10

volatile int total_balance = 0;
volatile int count; //indicate which thread should start to work

struct q q; //includes condition variable and the lock for protecting count

void do_work(void *arg){

    int i;
    int old;

    balance_t *b = (balance_t*) arg;
    int id = gettid();
    if(id < 0){

    	printf(1, "thread: error\n");
    	thread_exit();
        return;
    }
    char *name = b->name;
	int a = b->amount;

    //Before starting work, the thread is a receiver
    thread_mutex_lock(&q.m);

    while(count < id){ //not yet its turn

    	//i.e., instead of spinning on a spinlock or yielding the CPU in a mutex
    	//we would like the thread to sleep until certain contidion is met.
    	//A condition variable is used to allow a thread to sleep until a condition is true.
    	//The function primarily used for this is pthread_cond_wait()
    	//It takes two arguments; the first is a pointer to a condition variable
    	//and the second is a locked mutex.
    	thread_cond_wait(&q.cv, &q.m); //sleep on condition q.cv
    }

    thread_clear_signal(&q.cv); //let unqualified threads continue to sleep

    printf(1, "[thread %d: %s] Start to do work. Workload: %d\n", id, name, a);

    thread_mutex_unlock(&q.m);

    //since each time only one thread can work, so lock for protecting total_balance is unnecessary
    for (i = 0; i < a; i++) {
         old = total_balance;
         delay(100000);
         total_balance = old + 1;
    }

    //After finishing work, the thread becomes a sender
    thread_mutex_lock(&q.m);

    count++; //increment count to ask the next thread to work
    printf(1, "[thread %d: %s] Increment count to %d\n", id, name, count);

    //wake up all threads sleeping on condition q.cv
    printf(1, "[thread %d: %s] Work done. Signaling condition.\n", id, name);
    thread_cond_signal(&q.cv);

    thread_mutex_unlock(&q.m);

    thread_exit();

    return;
}

int main(int argc, char *argv[]) {

  balance_t b[THREAD_NUM];
  char *s[THREAD_NUM];
  int t[THREAD_NUM], r[THREAD_NUM];
  int correct_balance = 0;

  // Initialize
  thread_cond_init(&q.cv, "count");
  thread_mutex_init(&q.m, "count"); //lock for protecting count
  count = -1;

  int i;

  //int thread_create(void(*fcn)(void*), void *arg, void*stack)
  for(i = 0; i < THREAD_NUM; i++){

	  b[i].name[0] = 65+i;
	  b[i].name[1] = '\0';
	  b[i].amount = (i+1)*1000;
	  correct_balance = correct_balance + b[i].amount;
	  s[i] = sbrk(PGSIZE);
	  t[i] = thread_create(do_work, (void*)&b[i], (void*)s[i]);
  }

  //Before child threads start to work, the main thread is a sender
  thread_mutex_lock(&q.m);

  //initiate count
  count++; //ask the first thread to work
  printf(1, "[main thread] Increment count to %d\n", count);

  //wake up all threads sleeping on condition q.cv
  printf(1, "[main thread] Initialization done. Signaling condition.\n");
  thread_cond_signal(&q.cv);

  thread_mutex_unlock(&q.m);

  //After finishing initialization work, the main thread becomes a receiver
  thread_mutex_lock(&q.m);

  while(count < THREAD_NUM){

  	thread_cond_wait(&q.cv, &q.m);
  }

  thread_mutex_unlock(&q.m);

  //main thread collects exited child threads' pids
  for(i = 0; i < THREAD_NUM; i++){

	  r[i] = thread_join();
  }

  printf(1, "[main thread] Threads finished: ");

  for(i = 0; i < THREAD_NUM - 1; i++){

	  printf(1, "(%d):%d, ", t[i], r[i]);
  }
  printf(1, "(%d):%d\n", t[THREAD_NUM - 1], r[THREAD_NUM - 1]);

  printf(1, "[main thread] Shared balance: %d, correct balance: %d, diffs: %d\n", total_balance, correct_balance, total_balance - correct_balance);

  exit();
}
