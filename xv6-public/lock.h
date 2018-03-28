//you should define a simple lock data structure
struct thread_spinlock{

	char *name;
	uint locked;       // Is the lock held?
};

//you should define a simple mutex data structure
struct thread_mutex{

	char *name;
	uint locked;       // Is the lock held?
};

//define structure of a conditional variable
struct thread_cond{

	char *name;
	int signal;
};

//data structure shared by threads
struct q{

	   struct thread_cond cv;
	   struct thread_mutex m;
};

struct sem_t{

  char *name;
  int count;
  struct thread_mutex m;
  struct thread_cond cv;
};

uint xchg_1(volatile uint *, uint);
void thread_spin_init(struct thread_spinlock *, char *name);
void thread_spin_lock(struct thread_spinlock *);
void thread_spin_unlock(struct thread_spinlock *);
void thread_mutex_init(struct thread_mutex *, char *name);
void thread_mutex_lock(struct thread_mutex *);
void thread_mutex_unlock(struct thread_mutex *);
void thread_cond_init(struct thread_cond *, char *name);
void thread_cond_wait(struct thread_cond *, struct thread_mutex *);
void thread_cond_signal(struct thread_cond *);
void thread_clear_signal(struct thread_cond *cv);
int sem_init(struct sem_t *s, int value, char *name);
void sem_wait(struct sem_t *s);
void sem_post(struct sem_t *s);
