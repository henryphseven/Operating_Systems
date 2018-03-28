#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "thread.h"

/*
xv6 keeps all processes as an array inside the ptable data structure defined in proc.c.
To find the process you're working with you can simply loop through the entire array
and find the process which has the matching pid.
You can find an example for how it's done inside the kill() function (proc.c).
 */
struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

//add lock
struct {
  struct spinlock lock;
} pgtable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->thread = 0;

  //set up page table lock
  /*
  struct spinlock pgtable;
  p->pgtable = &pgtable;
  initlock(p->pgtable, "pgtable");
  */

  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.

//also refer to 2500 void userinit(void)
int thread_create(void(*fcn)(void*), void *arg, void *stack){

	  int i, pid;
	  struct proc *np;
	  struct proc *curproc = myproc();

	  // Allocate process.
	  //Keep them in the process array ---
	  //it's much easier than anything else,
	  if((np = allocproc()) == 0){
	    return -1;
	  }

	  /*
	  The thread_create() call should behave very much like fork,
	  except that instead of copying the address space to a new page directory,
	  clone initializes the new process so that the new process
	  and cloned process use the same page directory.
	  Thus, memory will be shared, and the two "processes" are really actually threads.
	   */

	  //just make up a flag saying that this entry is a thread.
	  np->thread = 1;
	  np->pgdir = curproc->pgdir;
	  np->sz = curproc->sz;
	  np->parent = curproc;

	  ///////////////////start to build thread's trapframe//////////////////////
	  *np->tf = *curproc->tf; //copy parent's trapframe content

	  //modify content of trapframe for thread to use

	  //The new process uses stack as its user stack,
	  //which is passed the given argument arg and uses a fake return PC (0xffffffff).
	  //The stack should be one page in size.
	  uint sp, ustack[2];

	  sp = (uint)stack + PGSIZE;

	  memset((char*)stack, 0, PGSIZE); //main may recycle stack page, so need to clear it first

	  //You should allocate a small data structure struct tls (thread local store)
	  //at the top of the stack of each thread.
	  struct tls t;
	  //You can count the number of threads already created in a process, right?
	  t.tid = np->pid - curproc->pid - 1; //tid starts from 0
	  sp -= sizeof(struct tls);
	  memmove((char*)sp, (char*)&t, sizeof(struct tls));

	  ustack[0] = 0xffffffff; //fake return PC (maybe curproc->tf->eip also works?)
	  //Arg should behave like a normal argument to a normal function, where do we put arguments?
	  //On the  stack, but on the user stack, not kernel.
	  ustack[1] = (uint)arg;
	  //void do_work(void *arg)

	  sp -= 2*4;
	  if(copyout(np->pgdir, sp, ustack, 2*4) < 0) return -1;

	  //initialize thread's stack pointer
	  np->tf->esp = sp;

	  // Clear %eax so that fork returns 0 in the child.
	  //np->tf->eax = 0;

	  //The new thread starts executing at the address specified by fcn .
	  //You should just set user's eip to start execution from *fcn.
	  //you should set it in the trapframe --
	  //that's the eip that the kernel restores before exiting back into the process
	  np->tf->eip = (uint)fcn;

	  //we will copy file descriptors in the same manner fork() does it.
	  for(i = 0; i < NOFILE; i++)
	    if(curproc->ofile[i])
	      np->ofile[i] = filedup(curproc->ofile[i]);
	  np->cwd = idup(curproc->cwd);

	  safestrcpy(np->name, curproc->name, sizeof(np->name));

	  pid = np->pid;

	  acquire(&ptable.lock);

	  np->state = RUNNABLE;

	  release(&ptable.lock);

	  //As with fork(), the PID of the new thread is returned to the parent.
	  return pid;
}

int thread_join(void){

	  struct proc *p;
	  int havekids, pid;
	  struct proc *curproc = myproc();

	  acquire(&ptable.lock);

	  for(;;){
	    // Scan through table looking for exited children.
	    havekids = 0;
	    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	      if(p->parent != curproc || p->thread != 1)
	        continue;
	      havekids = 1;

	      //You should however be careful and do not deallocate the page table
	      //of the entire process when one of the threads exits.
	      if(p->state == ZOMBIE){
	        // Found one.
	        pid = p->pid;
	        kfree(p->kstack);
	        p->kstack = 0;
	        p->pgdir = 0;
	        p->pid = 0;
	        p->parent = 0;
	        p->name[0] = 0;
	        p->killed = 0;
	        p->state = UNUSED;
	        p->thread = 0;
	        release(&ptable.lock);
	        return pid;
	      }
	    }

	    // No point waiting if we don't have any children.
	    if(!havekids || curproc->killed){
	      release(&ptable.lock);
	      return -1;
	    }

	    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
	    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
	  }

	  return -1;
}

void thread_exit(){

	  struct proc *curproc = myproc();
	  struct proc *p;
	  int fd;

	  // Close all open files.
	  for(fd = 0; fd < NOFILE; fd++){
	    if(curproc->ofile[fd]){
	      fileclose(curproc->ofile[fd]);
	      curproc->ofile[fd] = 0;
	    }
	  }

	  begin_op();
	  iput(curproc->cwd);
	  end_op();
	  curproc->cwd = 0;

	  acquire(&ptable.lock);

	  // Parent might be sleeping in wait().
	  wakeup1(curproc->parent);

	  // Pass abandoned children to init.
	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	    if(p->parent == curproc){
	      p->parent = initproc;
	      if(p->state == ZOMBIE)
	        wakeup1(initproc);
	    }
	  }

	  // Jump into the scheduler, never to return.
	  curproc->state = ZOMBIE;
	  sched();
	  panic("zombie exit");
}

//Where pid is a process identifier of the process which memory you're dumping,
//addr is the address inside the process memory from where the memory is dumped,
//buffer is the user allocated buffer where content of the user memory is saved,
//and size is the size of the memory region to dump.

//the corresponding kernel level interfaces must be implemented in proc.c
int dump(int pid, void *addr, void *buffer, int size){

	  uint i = 0, pa, n, off;
	  struct proc *p;
	  pte_t *pte;
	  char *a, *b;

	  acquire(&ptable.lock);

	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

	    if(p->pid == pid){

	    	b = (char*)buffer;
	    	memset(b, 0, size);

	    	/*
	    	Required, but just fetch the address as an integer
	    	and then check if it's withing the target process' address space,
	    	i.e., addr + size <= p->sz inside the dump system call
	    	where you already found the target process and are about to walk it's page table.
	    	 */
	    	if((uint)addr >= p->sz){

    			cprintf("dump: reach proc->sz %d of pid %d\n", p->sz, pid);
    			break;
    		}

	    	if((uint)addr+size > p->sz){

	    		size = p->sz - (uint)addr;
	    	}

	    	a = (char*)PGROUNDDOWN((uint)addr); //a must be page aligned
	    	off = (char*)addr - a;

	    	for(i = 0; i-off < size; i += PGSIZE){ //i increments by one page

	    		//you can iterate through the memory of the process page by page
	    		//until you see the first unmapped page
	    		//(your dump system call should return an error for that)
	    		if((pte = walkpgdir(p->pgdir, a+i, 0)) == 0){ //0 means unmapped

	    			cprintf("dump: see the first unmapped page of pid %d\n", pid);
	    			break;
	    		}

	    		pa = PTE_ADDR(*pte);

	    		if(i == 0){ //only move memory between offset and top of the page

	    			if(size < PGSIZE - off) n = size;
	    			else n = PGSIZE - off;

	    			memmove(b, P2V(pa)+off, n);
	    		}
	    		else{

	    			if(size - (i-off) < PGSIZE) n = size - (i-off);
	    			else n = PGSIZE;

	    			memmove(b+i-off, P2V(pa), n);
	    		}
	        } //for(i = 0; i < size; i += PGSIZE)

	        release(&ptable.lock);

	        if(i-off >= size) return size;
	        return i-off; //negative means failure
	    } // if(p->pid == pid)
	  } //for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)

	  release(&ptable.lock);
	  return -1;
}

/*
your new system call will have the following interface:
 int getprocinfo(int pid, struct uproc *up);

 Where pid is the process id of the target process,
 and struct uproc is a structure that describes the process,
 i.e., contains the following information about the process:
 process name, process id, parent process id, size of process memory,
 process state, whether process is waiting on a channel, and whether it's been killed.

 You will have to define the struct uproc
 and implement the ps utility by querying the system
 about all processes in the system.
 You should create a user-level program that calls your new date system call.
 */

int getnextpid(void)
{
	return nextpid;
}

int getprocinfo(int pid, struct uproc *up)
{
	//use part of procdump(void)
	static char *states[] = {
	[UNUSED] "unused",
	[EMBRYO] "embryo",
	[SLEEPING] "sleep ",
	[RUNNABLE] "runble",
	[RUNNING] "run ",
	[ZOMBIE] "zombie"
	};

	struct proc *p;

	acquire(&ptable.lock);

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

		if(p->pid == pid){

			memset((char *)up, 0, sizeof(struct uproc));

    		safestrcpy(up->name, p->name, sizeof(up->name));    // Process name (debugging)
    		up->pid = p->pid; // Process ID

    		if(p->pid == 1){ //the first user process

    			up->parent_pid = 0;
    		}
    		else{

    			up->parent_pid = p->parent->pid;              // Parent Process ID
    		}

    		up->sz = p->sz;                     // Size of process memory (bytes)

    		if(p->state >= 0 && p->state < NELEM(states) && states[p->state]){

    			safestrcpy(up->state, states[p->state], sizeof(up->state));
    		}
    		else{

    			safestrcpy(up->state, "???", sizeof(up->state));
    		}

    		// whether process is waiting on a channel => means the process is SLEEPING
    		if(p->state == SLEEPING){

    			up->waiting = 1;
    		}
    		else{

    			up->waiting = 0;
    		}

    		up->killed = p->killed;                  // If non-zero, have been killed
    		up->stack = PGROUNDDOWN(p->tf->esp);

    		release(&ptable.lock);
    		return 0;
		}
	}

	release(&ptable.lock);
	return -1;
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

void
pginit(void)
{
  initlock(&pgtable.lock, "pgtable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return old process size on success, -1 on failure.
int
growproc(int n)
{
  uint sz, addr; //sz: new process size, addr: old process size
  struct proc *curproc = myproc();

  acquire(&pgtable.lock); //need to acquire lock before changing size

  sz = curproc->sz;
  addr = sz;

  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){

	  //thread cannot deallocate memory to avoid free other threads' stacks
	  if(curproc->thread) return -1;

	  if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }

  if(curproc->thread){

	  curproc->parent->sz = sz;
  }
  curproc->sz = sz;

  //update sibling or child threads' process sizes if there are
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->thread && (p->parent == curproc->parent || p->parent == curproc)){

    	p->sz = curproc->sz;
    }
  }

  release(&pgtable.lock);

  switchuvm(curproc);
  return addr;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    //int wait() should wait for a child process that does not share the address space with this process.
      if(p->parent != curproc || p->thread == 1 || p->pgdir == curproc->pgdir)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        //It should also free the address space if this is last reference to it.
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->thread = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
