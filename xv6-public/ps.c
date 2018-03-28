#include "types.h"
#include "stat.h"
#include "user.h" //list all functions user can use
#include "syscall.h"

void ps(void)
{
	struct uproc *up; //user process
	up = malloc(sizeof(struct uproc));

	int nextpid = getnextpid(); //upper bound of pid
	int i;

	//5. Because the pid of a child process is zero, parent pid starts from 1?
	//Q5: Yes. that's the idea.
	for(i = 1; i < nextpid; i++){

		if(getprocinfo(i, up) == 0){
		//int getprocinfo(int pid, struct uproc *up);

					printf(1, "process name: %s\n", up->name);
					printf(1, "process id: %d\n", up->pid);

					if(up->pid == 1){

						printf(1, "parent process id: null\n");
					}
					else{

						printf(1, "parent process id: %d\n", up->parent_pid);
					}

					printf(1, "size of process memory: %d\n", up->sz);
					printf(1, "process state: %s\n", up->state);
					printf(1, "waiting on a channel: ");

					if(up->waiting){

						printf(1, "Yes");
					}
					else{

						printf(1, "No");
					}

					printf(1, "\n");
					printf(1, "has been killed: ");

					if(up->killed){

						printf(1, "Yes");
					}
					else{

						printf(1, "No");
					}

					printf(1, "\n");
					printf(1, "\n");
				}

		else{ //if(getprocinfo(i, up) != 0)

			 printf(1,"ps: pid %d has been terminated\n\n", i);
		}
	}

	free(up);
}
//int getprocinfo(int pid, struct uproc *up);

int main(int argc, char *argv[])
{
	  ps();
	  exit();
}
