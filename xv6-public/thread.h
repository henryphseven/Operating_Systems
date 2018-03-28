typedef struct{

   char name[32];
   int  amount;

} balance_t;

struct tls{

	int tid;
};

int gettid(void){

	/*
	 you can implement gettid() by rounding the value of the stack pointer up to the nearest page
	 and typecasting it to TLS.
	 */

	uint sp;
	sp = ((uint)&sp)/PGSIZE; //local variable is in the stack
	sp = (sp+1)*PGSIZE; //top of the stack
	sp -= sizeof(struct tls);

	struct tls *t = (struct tls*)sp;

	if(t->tid < 0) return -1;
	return t->tid;
}
