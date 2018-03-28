//Extra credit (10%): Fix sbrk() and malloc()
#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "lock.h"

#define THREAD_NUM 10

struct thread_mutex lock; //lock for protecting printf

void do_sbrk(void *arg){

    balance_t *b = (balance_t*) arg;
    int id = gettid();
    if(id < 0){

    	printf(1, "thread: error\n");
    	thread_exit();
        return;
    }
    char *name = b->name;
	int a = b->amount;

	thread_mutex_lock(&lock);
	printf(1, "[thread %d: %s (sbrk)] Old process size is %d before doing sbrk(%d)\n", id, name, (uint)sbrk(0), a);
	thread_mutex_unlock(&lock);

	char *mem = sbrk(a);

	thread_mutex_lock(&lock);
	printf(1, "[thread %d: %s (sbrk)] New process size is %d and returned address is %d\n", id, name, (uint)sbrk(0), (uint)mem);
	thread_mutex_unlock(&lock);

    thread_exit();
    return;
}

void do_malloc(void *arg){

    balance_t *b = (balance_t*) arg;
    int id = gettid();
    if(id < 0){

    	printf(1, "thread: error\n");
    	thread_exit();
        return;
    }
    char *name = b->name;
	int a = b->amount;

	thread_mutex_lock(&lock);
	printf(1, "[thread %d: %s (malloc)] Old process size is %d before doing malloc(%d)\n", id, name, (uint)sbrk(0), a);
	thread_mutex_unlock(&lock);

	void *mem = malloc(a);

	thread_mutex_lock(&lock);
	printf(1, "[thread %d: %s (malloc)] New process size is %d and returned address is %d\n", id, name, (uint)sbrk(0), (uint)mem);
	thread_mutex_unlock(&lock);

    thread_exit();
    return;
}

int main(int argc, char *argv[]) {

	  balance_t b[THREAD_NUM];
	  char *s[THREAD_NUM];
	  int t[THREAD_NUM], r[THREAD_NUM];

	  thread_mutex_lock(&lock);
	  printf(1, "[main thread] Old process size is %d before multi-threading\n", (int)sbrk(0));
	  thread_mutex_unlock(&lock);

	  thread_mutex_init(&lock, "printf");

	  int i;

	  //int thread_create(void(*fcn)(void*), void *arg, void*stack)
	  for(i = 0; i < THREAD_NUM; i++){

		  b[i].name[0] = 65+i;
		  b[i].name[1] = '\0';
		  b[i].amount = PGSIZE;
		  s[i] = sbrk(PGSIZE);

		  //even threads do sbrk and odd threads do malloc
		  if(i%2 == 0) t[i] = thread_create(do_sbrk, (void*)&b[i], (void*)s[i]);
		  else t[i] = thread_create(do_malloc, (void*)&b[i], (void*)s[i]);

		  thread_mutex_lock(&lock);
		  printf(1, "[main thread] New process size is %d after creating thread %d\n", (int)sbrk(0), i);
		  thread_mutex_unlock(&lock);
	  }

	  //main thread collects exited child threads' pids
	  for(i = 0; i < THREAD_NUM; i++){

		  r[i] = thread_join();
	  }

	  printf(1, "[main thread] Threads finished: ");

	  for(i = 0; i < THREAD_NUM - 1; i++){

		  printf(1, "(%d):%d, ", t[i], r[i]);
	  }
	  printf(1, "(%d):%d\n", t[THREAD_NUM - 1], r[THREAD_NUM - 1]);

	  printf(1, "[main thread] New process size is %d after all child threads exited\n", (int)sbrk(0));

	  exit();
}
