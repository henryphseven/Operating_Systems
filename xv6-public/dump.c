#include "types.h"
#include "stat.h"
#include "user.h" //list all functions user can use
#include "syscall.h"

//You should implement a new user application named dump and that should be implemented in dump.c
void dump2(void){

	/* Fork a new process to play with */
	/* We don't have a good way to list all pids in the system
	   so forking a new process works for testing */
	int pid = fork();

	if (pid == 0) {
	    /* child spins and yields */
	    while (1) {
	       sleep(5);
	    };
	}

	//Below is parent process
	uint stack;
	stack = ((uint)&pid)/PGSIZE; //local variable is in the stack
	stack = stack*PGSIZE;

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

int main(int argc, char *argv[])
{
	dump2();
	exit();
}
