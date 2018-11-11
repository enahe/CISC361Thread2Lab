#include "t_lib.h"

const useconds_t timeout = 1;  
struct tcb {
	  int         thread_id;
          int         thread_priority;
	  ucontext_t  *thread_context;
	  struct tcb *next;
       };

       typedef struct tcb tcb;
tcb *running;
tcb *highready;
tcb *ready;

void sig_handler(int sig_no) {
sigset(SIGALRM, sig_handler);
t_yield();
}

void t_yield()
{
  sighold(SIGALRM);

  tcb* tmp;
  tmp =  running; //store the currently running htread in tmp
  if (tmp != NULL) {
  tmp->next = NULL;
  }
  if (highready != NULL) { //only yield if there is something in ready queue
	running = highready; //update running to first thread in ready queue
   	highready = highready->next; //update ready to next thread
  	running->next = NULL;
  	tcb* iter;
	iter = highready;
	if (iter == NULL) //if there is nothing else in ready queue
		highready = tmp;
	else { 
  		while (iter->next != NULL) //loop till end of queue
			iter = iter->next;
  		iter->next = tmp; //add tmp to end of queue
	}
	ualarm(timeout, 0);
	swapcontext(tmp->thread_context, running->thread_context);
        sigrelse(SIGALRM);
  }
  else {
        running = ready; //update running to first thread in ready queue
   	ready = ready->next; //update ready to next thread
  	running->next = NULL;
  	tcb* iter;
	iter = ready;
	if (iter == NULL) //if there is nothing else in ready queue
		ready = tmp;
	else { 
  		while (iter->next != NULL) //loop till end of queue
			iter = iter->next;
  		iter->next = tmp; //add tmp to end of queue
	}
	ualarm(timeout, 0);
	swapcontext(tmp->thread_context, running->thread_context);
        sigrelse(SIGALRM);

  }

}
 /* Initialize the thread library by setting up the "running" 
and the "ready" queues, creating TCB of the "main" thread, and inserting it into the running queue. */
void t_init()
{
        sigset(SIGALRM, sig_handler);
	tcb *tmp = (tcb*) malloc(sizeof(tcb));
	tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));
        ualarm(timeout, 0);
	getcontext(tmp->thread_context);
	tmp->next = NULL;
	tmp->thread_id = 0;
        tmp->thread_priority = 1;
	running = tmp;
	ready = NULL; 
        highready = NULL;
}
/* Create a thread with priority pri, start function func with argument thr_id 
as the thread id. Function func should be of type void func(int). TCB of the newly
 created thread is added to the end of the "ready" queue; the parent thread calling
t_create() continues its execution upon returning from t_create(). */

int t_create(void (*fct)(int), int id, int pri)
{ 
  size_t sz = 0x10000;
  tcb* tmp = (tcb*) malloc(sizeof(tcb));
  tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));

  getcontext(tmp->thread_context);
/***
  uc->uc_stack.ss_sp = mmap(0, sz,
       PROT_READ | PROT_WRITE | PROT_EXEC,
       MAP_PRIVATE | MAP_ANON, -1, 0);
***/
  tmp->thread_context->uc_stack.ss_sp = malloc(sz);  /* new statement */
  tmp->thread_context->uc_stack.ss_size = sz;
  tmp->thread_context->uc_stack.ss_flags = 0;
  tmp->thread_context->uc_link = running->thread_context; 
  makecontext(tmp->thread_context, (void (*)(void)) fct, 1, id);
  tmp->thread_id = id;
  tmp->thread_priority = pri;
  tmp->next = NULL;

  if (ready == NULL)
	ready = tmp;
  else {
        if (pri == 1) {
	tcb* t = ready;
	while(t->next!=NULL) {
		t = t->next;
	}
	t->next = tmp;
  }
       else {
       tcb* t = highready;
	while(t->next!=NULL) {
		t = t->next;
	}
	t->next = tmp;
 }
}
}

/* Terminate the calling thread by removing (and freeing) its TCB from the
"running" queue, and resuming execution of the thread in the head of the 
"ready" queue via setcontext(). */
void t_terminate(){

	tcb* tmp;
	tmp = running;
        if (highready == NULL) {
	running = ready;
	if (ready!=NULL)
		ready = ready->next;
	free(tmp->thread_context->uc_stack.ss_sp);
	free(tmp->thread_context);
	free(tmp);
	setcontext(running->thread_context);
        }
        else {
        running = highready;
	if (highready!=NULL)
		highready = highready->next;
	free(tmp->thread_context->uc_stack.ss_sp);
	free(tmp->thread_context);
	free(tmp);
	setcontext(running->thread_context);

      }

}

void t_shutdown(){
        if (highready!=NULL) {
		tcb* tmp;
		tmp = highready;
		while(tmp != NULL) {
			highready = highready->next;
			free(tmp->thread_context->uc_stack.ss_sp);
			free(tmp->thread_context);
			free(tmp);
			tmp = highready;
		}
	}
	if (ready!=NULL) {
		tcb* tmp;
		tmp = ready;
		while(tmp != NULL) {
			ready = ready->next;
			free(tmp->thread_context->uc_stack.ss_sp);
			free(tmp->thread_context);
			free(tmp);
			tmp = ready;
		}
	}
	free(running->thread_context);
	free(running);	
}


