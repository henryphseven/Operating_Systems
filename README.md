Operating System Name: xv6

Original source code:
https://github.com/mit-pdos/xv6-public

Please refer to the following link for more details: 
https://pdos.csail.mit.edu/6.828/2016/xv6/book-rev9.pdf

How to compile and run xv6 in Linux:

1. Install QEMU: In the terminal, type "sudo apt-get install qemu". And then type "sudo apt-get install libc6-dev:i386".

2. Download the files into a folder. 
In the terminal, change the directory to that folder.
Type "make" to compile the code.
And then type “make qemu-nox” to run xv6.

Please refer to the following links for more details:
- http://www.ics.uci.edu/~aburtsev/238P/hw/xv6-setup.html
- https://namangt68.github.io/xv6/os/ubuntu/2016/05/08/quick-install-xv6.html

New features implemented: 
(The name of the source file corresponds to a command which is accepted by the xv6 shell. For example, typing "ps" will execute the program "ps.c" and print all processes running in the system and information about them.)

1. ps.c: implement a new system call getprocinfo() (proc.c) which returns information of a process

The program prints all processes running in the system and information about them.

2. dumppid.c: implement a new system call dump() (proc.c) which dumps memory of a process on the console

Typing "dumppid <process_id>" will dump memory of the process with the matching process id on the console.

3. thread.c: implement new system calls thread_create(), thread_join(), and thread_exit() (proc.c) which create a thread, wait for a child thread to exit, and terminate the current thread, respectively; implement new user functions thread_spin_init(), thread_spin_lock(), and thread_spin_unlock() (lock.c) to protect critical sections using spinlocks

The program creates two threads that execute the same do_work() function concurrently. The do_work() function in both threads updates the shared variable total_balance.

4. thread_m.c: implement new user functions thread_mutex_init(), thread_mutex_lock(), and thread_mutex_unlock() (lock.c) to protect critical sections using mutexes

Same as above. The only difference is replacing spinlocks with mutexes.

5. thread_cv.c: implement new user functions thread_cond_init(), thread_cond_wait(), and thread_cond_signal() (lock.c) to synchronize threads using conditional variables

The program creates 10 threads and has a global variable "count".
Only when count = tid can the thread start to work.
Before starting work, the thread is a receiver; after finishing work, the thread becomes a sender. 
Initially count is -1.
Main thread increments the count and wakes up all threads sleeping on condition.
But only thread 0 satisfies the condition "count = tid", so only thread 0 starts to work.
After thread 0 finishes work, it increments the count and wakes up all threads sleeping on condition.
But only thread 1 satisfies the condition "count = tid", so only thread 1 starts to work.
The cycle repeats until all child threads finish work.
Thead 9 finishes work and wakes up all threads sleeping on condition, only main thread at this time.
Main thread wakes up, finds that all child threads finish work, and continues its job.

6. thread_sem.c: implement new user functions sem_init(), sem_wait(), and sem_post() (lock.c) to synchronize threads using semaphores 

The program creates 10 threads.
Odd threads are producers (increase total_balance) and even threads are consumers (decrease total_balance).
Because queue size (N) is only 3, if there have been 3 producers/cosumers working, others need to wait.
Semaphores emptyCount and fillCount are used to coordinate their work.

7. thread_mem.c: fix sbrk() (sysproc.c) by modifying growproc() (proc.c) and malloc() (umalloc.c)

The program creates 10 threads.
Even threads do sbrk(PGSIZE) and odd threads do malloc(PGSIZE).
Locks are added to growproc (pgtable.lock) and malloc (lock) to restrict threads' access to the two functions.
Parent thread and child threads' process sizes are also synchronized to maintain the consistency of their page tables.

8. thread_ptv.c: implement a new user function gettid() (thread.h) which returns thread ID

The program creates 10 threads and declares per-thread variables - name and amount.
gettid() is used to access the variables.

9. thread_ptvm.c: implement new macros DEFINE_PER_THREAD(type, name) and per_thread(name) (thread.h)

Same as above. The only difference is replacing gettid() with macros.

10. big.c: implement linked-list file addressing by modifying bmap() (fs.c) and iappend() (mkfs.c)

Originally the size of a single file in xv6 is limited to 140 sectors, or 71,680 bytes. Linked-list file addressing allows files of infinite length. The program writes 4 files: 1MB, 2MB, 4MB, and 8MB, and then reads them.

Please refer to the following links for more details:
- http://www.ics.uci.edu/~aburtsev/238P/hw/hw3-dump-pid-syscall.html
- http://www.ics.uci.edu/~aburtsev/238P/hw/hw4-threads.html
- http://www.ics.uci.edu/~aburtsev/238P/hw/hw5-infinite-files.html
