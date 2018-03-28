//Implement Spinlocks
#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "lock.h"

volatile int total_balance = 0;

struct thread_spinlock lock; //lock for protecting total_balance
struct thread_spinlock lock_1; //lock for protecting printf

void do_work(void *arg){

    int i;
    int old;

    balance_t *b = (balance_t*) arg;

    thread_spin_lock(&lock_1);
    printf(1, "Starting do_work: %s\n", b->name);
    thread_spin_unlock(&lock_1);

    for (i = 0; i < b->amount; i++) {
    	 thread_spin_lock(&lock);
         old = total_balance;
         delay(100000);
         total_balance = old + 1;
         thread_spin_unlock(&lock);
    }

    thread_spin_lock(&lock_1);
    printf(1, "Done: %s\n", b->name);
    thread_spin_unlock(&lock_1);

    thread_exit();

    return;
}

int main(int argc, char *argv[]) {

  balance_t b1 = {"b1", 3200};
  balance_t b2 = {"b2", 2800};

  void *s1, *s2;
  int t1, t2, r1, r2;

  s1 = malloc(PGSIZE);
  s2 = malloc(PGSIZE);

  thread_spin_init(&lock, "total_balance");
  thread_spin_init(&lock_1, "printf");

  //int thread_create(void(*fcn)(void*), void *arg, void*stack)
  t1 = thread_create(do_work, (void*)&b1, s1);
  t2 = thread_create(do_work, (void*)&b2, s2);

  r1 = thread_join();
  r2 = thread_join();

  printf(1, "Threads finished: (%d):%d, (%d):%d, shared balance: %d\n",
      t1, r1, t2, r2, total_balance);

  exit();
}
