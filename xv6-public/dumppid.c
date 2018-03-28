#include "types.h"
#include "stat.h"
#include "user.h" //list all functions user can use
#include "syscall.h"

void dumppid2(int pid)
{
	uint stack;

	if(pid == -1){

		  /* Fork a new process to play with */
		  /* We don't have a good way to list all pids in the system
		     so forking a new process works for testing */
		  pid = fork(); //for parent, pid becomes child process's pid

		  if (pid == 0) {
		    /* child spins and yields */
		    while (1) {
		       sleep(5);
		    };
		  }

		  stack = ((uint)&pid)/PGSIZE; //local variable is in the stack
		  stack = stack*PGSIZE;
	}
	else{

		  struct uproc *up; //user process
		  up = malloc(sizeof(struct uproc));

		  if(pid == 0 || getprocinfo(pid, up) == -1){ //check the pid is valid

			printf(1,"dumppid: pid %d does not exist or has been terminated\n", pid);
			free(up);
		    return;
		  }

		  stack = up->stack;
		  free(up);
	}

	int size = PGSIZE;
	char *addr = (char *)0x0;
	void *buffer = malloc(size);
	int n;
	uint i = 0;

 	/* parent dumps memory of the child */
 	//You will have to dump the entire memory of the process on the console,
	//i.e., starting from address 0 to proc->sz.
	while(1){

		n = dump(pid, addr+i, buffer, size); //n: actual dumped memory size
		//int dump(int pid, void *addr, void *buffer, int size)

		if(n <= 0) break;

		printmemory(addr+i, buffer, n, stack); //it is defined in "ulib.c"
		//void printmemory(void* buffer, int n, uint stack)

		i = i + n;
	}

	if(i == 0 && n <= 0) printf(1, "dump: fail to dump memory of pid %d\n", pid);

	free(buffer);
	return;
}
//int dump(int pid, void *addr, void *buffer, int size)

int main(int argc, char *argv[])
{
	int pid;

	if(argc <= 1){

		pid = -1;
	}
	else{

		pid = atoi(argv[1]);
	}

	dumppid2(pid);
	exit();
}
