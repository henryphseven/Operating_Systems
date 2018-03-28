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

http://www.ics.uci.edu/~aburtsev/238P/hw/xv6-setup.html

https://namangt68.github.io/xv6/os/ubuntu/2016/05/08/quick-install-xv6.html


New features implemented: (The name of the source file corresponds to a command which is accepted by the xv6 shell.)

1. ps.c: implement a new system call "getprocinfo" which returns information for a process

The program prints all processes running in the system and information about them.

2. dumppid.c: implement a new system call "dump" which dumps memory of a process on the console

Typing "dumppid <process_id>" will dump memory of the process with the matching process id on the console.

3. thread.c: implement threads and spinlocks

The program creates two threads that execute the same do_work() function concurrently. The do_work() function in both threads updates the shared variable total_balance.

4. thread_m.c: implement mutexes

Same as above. The only difference is replacing spinlocks with mutexes.

5. thread_cv.c: implement conditional variables

In this program, I create 10 threads and define a global variable "count".
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

6. thread_sem.c: implement semaphores and the producer consumer queue of N elements

In this program, I create 10 threads.
Odd threads are producers (increase total_balance) and even threads are consumers (decrease total_balance).
Because queue size (N) is only 3, if there have been 3 producers/cosumers working, others need to wait.
I use semaphores emptyCount and fillCount to coordinate their work.

7. thread_mem.c: fix sbrk() and malloc()

In this program, I create 10 threads.
Even threads do sbrk(PGSIZE) and odd threads do malloc(PGSIZE).
I add locks to growproc (pgtable.lock) and malloc (lock) to restrict threads' access to the two functions.
Besides, I also modify growproc to synchronize parent thread and child threads' process sizes.

8. thread_ptv.c: implement gettid()

In this program, I create 10 threads and declare per-thread variables - name and amount.
And then I use gettid() to access the variables.

9. thread_ptvm.c: implement macros DEFINE_PER_THREAD(type, name) and per_thread(name)

Same as above. The only difference is replacing gettid() with macros.

10. big.c: implement a linked-list file addressing for files of infinite length

The program writes 4 files: 1MB, 2MB, 4MB, and 8MB, and then reads them.

Please refer to the following links for more details:

http://www.ics.uci.edu/~aburtsev/238P/hw/hw3-dump-pid-syscall.html

http://www.ics.uci.edu/~aburtsev/238P/hw/hw4-threads.html

http://www.ics.uci.edu/~aburtsev/238P/hw/hw5-infinite-files.html
