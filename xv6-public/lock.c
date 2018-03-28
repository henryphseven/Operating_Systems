#include "types.h"
#include "stat.h"
#include "user.h"
#include "lock.h"

uint
xchg_1(volatile uint *addr, uint newval)
{
  uint result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

//and implement three functions that:
//1) initialize the lock to the correct initial state
void thread_spin_init(struct thread_spinlock *lk, char *name){

	lk->name = name;
	lk->locked = 0;
}

//2) a funtion to acquire a lock
void thread_spin_lock(struct thread_spinlock *lk){

	  // The xchg is atomic.
	  while(xchg_1(&lk->locked, 1) != 0)
	    ;

	  // Tell the C compiler and the processor to not move loads or stores
	  // past this point, to ensure that the critical section's memory
	  // references happen after the lock is acquired.
	  __sync_synchronize();
}

//3) a function to release it
void thread_spin_unlock(struct thread_spinlock *lk){

	  if(!lk->locked){

		  printf(1, "The lock is not acquired\n");
		  return;
	  }

	  // Tell the C compiler and the processor to not move loads or stores
	  // past this point, to ensure that all the stores in the critical
	  // section are visible to other cores before the lock is released.
	  // Both the C compiler and the hardware may re-order loads and
	  // stores; __sync_synchronize() tells them both not to.
	  __sync_synchronize();

	  // Release the lock, equivalent to lk->locked = 0.
	  // This code can't use a C assignment, since it might
	  // not be atomic. A real OS would use C atomics here.
	  asm volatile("movl $0, %0" : "+m" (lk->locked) : );
}

//and implement three functions that:
//1) initialize the lock to the correct initial state
void thread_mutex_init(struct thread_mutex *m, char *name){

	m->name = name;
	m->locked = 0;
}

//2) a funtion to acquire a lock
void thread_mutex_lock(struct thread_mutex *m)
{
	  // The xchg is atomic.
	  while(xchg_1(&m->locked, 1) != 0){

		  //Since xv6 doesn't have an explicit yield(0) system call, you can use sleep(1) instead
		  sleep(1);
	  }

	  //printf(1, "lock %s acquired\n", m->name);

	  // Tell the C compiler and the processor to not move loads or stores
	  // past this point, to ensure that the critical section's memory
	  // references happen after the lock is acquired.
	  __sync_synchronize();
}

//3) a function to release it
void thread_mutex_unlock(struct thread_mutex *m)
{
  if(!m->locked){

	  //printf(1, "lock %s is not acquired\n", m->name);
	  return;
  }

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to m->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m" (m->locked) : );

  //printf(1, "lock %s released\n", m->name);
}

//thread_cond_init(&q->cv);
void thread_cond_init(struct thread_cond *cv, char *name){

	cv->name = name;
	cv->signal = 0;
}

//thread_cond_wait(&q->cv, &q->m)
void thread_cond_wait(struct thread_cond *cv, struct thread_mutex *m){

	/*
	 The call pthread_cond_wait performs three actions:
	1. unlock the mutex
	2. waits (sleeps until pthread_cond_signal is called on the same condition variable)
	3. Before returning, locks the mutex
	 */

	  /*
	  When invoked, pthread_cond_wait() unlocks the mutex and then pauses execution of its thread.
	  It will now remain paused until such time as some other thread wakes it up.
	  These operations are "atomic;" they always happen together,
	  without any other threads executing in between them.
	  */
	  if(m->locked == 0)
	    printf(1, "thread_cond_wait without m\n");
	  else thread_mutex_unlock(m);

	  // Go to sleep.
	  // sleep until a condition is true
	  while(!cv->signal){

		  sleep(1);
	  }

	  // Reacquire original lock.
	  thread_mutex_lock(m);
}

//thread_cond_signal(&q->cv);
void thread_cond_signal(struct thread_cond *cv){

	cv->signal = 1;
}

void thread_clear_signal(struct thread_cond *cv){

	cv->signal = 0;
}

int sem_init(struct sem_t *s, int value, char *name) {

  s->name = name;
  s->count = value;
  thread_mutex_init(&s->m, name);
  thread_cond_init(&s->cv, name);
  return 0;
}

void sem_wait(struct sem_t *s) {

	thread_mutex_lock(&s->m);

	while (s->count == 0) {

		thread_cond_wait(&s->cv, &s->m); /*unlock mutex, wait, relock mutex*/
	}

	s->count--;
	//printf(1, "%s is decremented and becomes %d\n",s->name, s->count);

	thread_mutex_unlock(&s->m);
}

void sem_post(struct sem_t *s) {

	thread_mutex_lock(&s->m);

	s->count++;
	//printf(1, "%s is incremented and becomes %d\n",s->name, s->count);

	/* Did we increment from zero to one- time to signal a thread sleeping inside sem_post */
	if (s->count == 1) /* Wake up one waiting thread!*/
	     thread_cond_signal(&s->cv);
    /* A woken thread must acquire the lock, so it will also have to wait until we call unlock*/

	thread_mutex_unlock(&s->m);
}
