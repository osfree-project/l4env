/* $Id$
 *
 * this is a l4 Example (a theoretical contribution to Fiasco ;-)
 * 
 * create 128 threads for one task and let a message cycle
 * through all these threads
 */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <stdio.h>

static int debugwait = 0;


/* the stack for the other 127 threads */
char * stack2[127][2048];


/* nicely report ipc errors */
static
void ipc_error(char * msg, int error)
{
  if(error == 0){
    printf("%s : ok\n", msg);
  }
  else {
    if( error == L4_IPC_ENOT_EXISTENT ){
      printf("%s : L4_IPC_ENOT_EXISTENT "
	     "(Non-existing destination or source)\n", msg);
    }
    if( error == L4_IPC_RETIMEOUT ){
      printf("%s : L4_IPC_RETIMEOUT (Timeout during receive)\n", msg);
    }
    if( error == L4_IPC_SETIMEOUT ){
      printf("%s : L4_IPC_SETIMEOUT (Timeout during send)\n", msg);
    }
    if( error == L4_IPC_RECANCELED ){
      printf("%s : L4_IPC_RECANCELED (Receive operation cancelled)\n", msg);
    }
    if( error == L4_IPC_SECANCELED ){
      printf("%s : L4_IPC_SECANCELED (Send operation cancelled)\n", msg);
    }
    if( error == L4_IPC_REMAPFAILED ){
      printf("%s : L4_IPC_REMAPFAILED (Map failed in send)\n", msg);
    }
    if( error == L4_IPC_SEMAPFAILED ){
      printf("%s : L4_IPC_SEMAPFAILED (Map failed in receive)\n", msg);
    }
    if( error == L4_IPC_RESNDPFTO ){
      printf("%s : L4_IPC_RESNDPFTO (Send pagefault timeout)\n", msg);
    }
    if( error == L4_IPC_SESNDPFTO ){
      printf("%s : L4_IPC_SESNDPFTO (?)\n", msg);
    }
    if( error == L4_IPC_RERCVPFTO ){
      printf("%s : L4_IPC_RERCVPFTO (?)\n", msg);
    }
    if( error == L4_IPC_SERCVPFTO ){
      printf("%s : L4_IPC_SERCVPFTO (Receive pagefault timeout)\n", msg);
    }
    if( error == L4_IPC_REABORTED ){
      printf("%s : L4_IPC_REABORTED (Receive operation aborted)\n", msg);
    }
    if( error == L4_IPC_SEABORTED ){
      printf("%s : L4_IPC_SEABORTED (Send operation aborted)\n", msg);
    }
    if( error == L4_IPC_REMSGCUT ){
      printf("%s : L4_IPC_REMSGCUT (Received message cut)\n", msg);
    }
    if( error == L4_IPC_SEMSGCUT ){
      printf("%s : L4_IPC_SEMSGCUT (?)\n", msg);
    }
  }
}


/* wait for 255 * 4 ^ (15 - exp)  micro seconds 
 * on my platform inside bochs exp==8 gives about one second
 */
static
void sit_and_wait(int exp)
{
  l4_ipc_sleep(L4_IPC_TIMEOUT(255,exp,255,exp,0,0));
}


/* this gets started for the other 128 threads */
static void othread(void)
{
  l4_threadid_t src, th, my_preempter, my_pager;
  l4_msgdope_t result;
  l4_umword_t msg, ignore;
  l4_timeout_t t4s = L4_IPC_TIMEOUT(255,8,255,8,0,0);
  int res;
  l4_addr_t sp;

  /* get my thread id */
  th = l4_myself();

  my_preempter = L4_INVALID_ID;
  my_pager = L4_INVALID_ID;

  /* get my preempter & pager */
  l4_thread_ex_regs( th, -1, -1, 
		     &my_preempter, 	/* preempter */
		     &my_pager, 	/* pager */
		     &ignore, 		/* flags */
		     &ignore, 		/* old ip */
		     &sp		/* old sp */
		     );
  printf("This is thread, task %u, thread %u, stack at %lx\n", 
	 th.id.task, th.id.lthread, sp);

  /* cycle get a message at write it to the next thread */
  for(;;)
    {
      do {
	res = l4_ipc_wait(&src,  L4_IPC_SHORT_MSG, &msg ,  &ignore,  
			  L4_IPC_NEVER, &result );
	if(res){
	  ipc_error("thread rcv", res);
	}
      } 
      while(res != 0);

      printf("%u(%u) : got %lu from task %u thread %u\n", 
	     th.id.task, th.id.lthread,
	     msg, src.id.task, src.id.lthread);

      if (debugwait)
	sit_and_wait(12);

      src.id.lthread = th.id.lthread + 1;

      do {
	// printf("send to task %u thread %u\n", src.id.task, src.id.lthread);
	res = l4_ipc_send(src, 0, msg, ignore, t4s, &result);
	if(res){
	  ipc_error("thread snd", res);
	}
      } 
      while(res != 0);
    }
}
      

int main(void)
{
  l4_threadid_t my_id, o_id, my_preempter, my_pager, new_pager;
  l4_msgdope_t result;
  l4_umword_t msg, ignore;
  int i, res;

  my_preempter = L4_INVALID_ID;
  my_pager = L4_INVALID_ID;

  /* get my id */
  o_id = my_id = l4_myself(); 

  /* get preempter & pager */
  l4_thread_ex_regs( my_id, -1, -1, 
		     &my_preempter, 	/* preempter */
		     &my_pager, 	/* pager */
		     &ignore, 		/* flags */
		     &ignore, 		/* old ip */
		     &ignore		/* old sp */
		     );

  printf("This is main, task %u, thread %u, "
	 "preempter %u(%u), pager %u(%u)\n", 
	 my_id.id.task, my_id.id.lthread, 
	 my_preempter.id.task, my_preempter.id.lthread,
	 my_pager.id.task, my_pager.id.lthread );

  if (debugwait)
    sit_and_wait(8);

#ifdef L4API_l4x0
#define MAX_THREADS 64
#else
#define MAX_THREADS 128
#endif

  /* create all the tasks */
  for(i = 1; i < MAX_THREADS; i++){
    o_id.id.lthread = i;
    new_pager = my_pager;
    my_preempter = L4_INVALID_ID;
    l4_thread_ex_regs( o_id, (l4_umword_t)othread, 
		       (l4_umword_t)( stack2[i-1] +2048), 
		       &my_preempter, 	/* preempter */
		       &new_pager, 	/* pager */
		       &ignore, 	/* flags */
		       &ignore, 	/* old ip */
		       &ignore		/* old sp */
		       );
  }

  if (debugwait)
    sit_and_wait(8);
  
  /* start the cycle */
  msg = 0;
  for (;;)
    {
      o_id.id.lthread = 1;
      do {
	res = l4_ipc_reply_and_wait
	  (o_id, L4_IPC_SHORT_MSG, msg, ignore, &o_id, 
	   L4_IPC_SHORT_MSG, &msg, &ignore, 
	   L4_IPC_NEVER, &result);
	if(res != 0){
	  printf("[%u:%x]\n", my_id.id.lthread, res);
	}
      } 
      while(res != 0);
      msg++;
    }
}

