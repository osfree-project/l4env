/* $Id$
 *
 * this is a l4 Example (a theoretical contribution to Fiasco ;-)
 * 
 * Create as much tasks and threads as possible until the kernel
 * panics. The program does the following:
 * 
 * 1. get all available physical memory from the rmgr, map it 
 *    consecutively at client_stack_base_addr
 * 2. get all available task creation rights
 * 3. get kernel info page
 * 4. start some utility threads (pager, spinner, counter, locker)
 * 5. create some tasks (number_of_tasks * ping_pong_task, initiator_task)
 * 6. start to remap the pages until memory runs out
 *  
 * The remapping works in a cascaded way:
 *   - ping_pong_task remap the same page cyclicly until an error because 
 *     the nesting level is too high
 *   - when the nesting error occurs the initiator_task maps the same page 
 *     again to the first ping_pong_task in the cycle
 *   - at some stage the mapping database for this page is full, then the 
 *     initiator task gets a new physical page from the main program 
 *     and the game starts again
 * 
 * 
 * There are four threads in the main task:
 * 
 * lock_thread: provide a _very_ simple lock, that allows all 
 * 		the other threads to syncronise their output
 * 
 * pager_thread: pager for tasks
 * 
 * counter_thread: count the threads created and print a message once a while
 * 
 * spinner_thread: the thread that creates all tasks, 
 * 		   doing this in a separate thread allows to 
 * 		   manipulate its priority
 */


#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/rmgr/librmgr.h> 
#include <l4/sys/kernel.h>
#include <l4/sigma0/kip.h>
#include <l4/sigma0/sigma0.h>
#include <stdio.h>

/* number of tasks to create */
static int number_of_tasks = 10;

/* the virtual address range of this program */
extern char _end, _start;
extern char _stext, _etext;

/* debug variables, set to nonzero to get more output */
static int debug = 0;
static int lockdebug = 0;
static int debugpager = 0;
static int debugwait = 0;


/* base address for physical memory */
l4_addr_t client_stack_base_addr = 0x80000000;

/* base address for getting mappings */
l4_addr_t ping_pong_base_addr =    0x00400000;

/* virtual address of kernel info page */
static l4_kernel_info_t * kinfo = (void *)0x00002000;


/* thread ids of main task */
#define LOCK_THREAD 1
#define PAGER_THREAD 2
#define COUNTER_THREAD 3
#define SPINNER_THREAD 4


/* l4 task ids for threads of main task */
static l4_threadid_t lock_thread_id = L4_INVALID_ID;
static l4_threadid_t pager_thread_id = L4_INVALID_ID;
static l4_threadid_t counter_thread_id = L4_INVALID_ID;
static l4_threadid_t spinner_thread_id = L4_INVALID_ID;


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


/* shut down this thread */
static
void wait_forever(void)
{
  l4_ipc_sleep(L4_IPC_NEVER);
}
     

/* get thread number, preempter, pager, and ESP of calling thread */
static
int get_thread_ids(l4_threadid_t *my_id, 
		   l4_threadid_t *my_preempter, 
		   l4_threadid_t *my_pager,
		   l4_addr_t *esp)
{
  l4_umword_t ignore, _esp;
  l4_threadid_t _my_id, _my_preempter, _my_pager;

  _my_id = l4_myself(); 

  _my_preempter = L4_INVALID_ID;
  _my_pager = L4_INVALID_ID;

  l4_thread_ex_regs( _my_id, -1, -1,	     /* id, EIP, ESP */
		     &_my_preempter,	     /* preempter */
		     &_my_pager,	     /* pager */
		     &ignore,		     /* flags */
		     &ignore,		     /* old ip */
		     &_esp		     /* old sp */
		     );

  if(my_id) *my_id = _my_id;
  if(my_preempter) *my_preempter = _my_preempter;
  if(my_pager) *my_pager = _my_pager;
  if(esp) *esp = _esp;
  return 0;
}


/* acquire lock */
static void lock(void)
{
  int res;
  l4_umword_t ignore =0;
  l4_msgdope_t dope;

  if(lock_thread_id.lh.low == L4_INVALID_ID.lh.low)
    {
      printf("Lock thread not initialised\n");
    }
  else
    {
      res = l4_ipc_send(lock_thread_id,     /* dest */
                        L4_IPC_SHORT_MSG,   /* snd desc */
                        ignore, ignore,	    /* data */
                        L4_IPC_NEVER,	    /* timeout */
                        &dope );
      if(lockdebug)
	{
	  sit_and_wait(8);
	}
      if(res)
	ipc_error("lock", res);
    }
}

/* release lock */
static void unlock(void)
{
  int res;
  l4_umword_t ignore;
  l4_msgdope_t dope;

  if(lock_thread_id.lh.low == L4_INVALID_ID.lh.low)
    {
      printf("Unlock thread not initialised\n");
    }
  else
    {
      res = l4_ipc_receive(lock_thread_id,    /* dest */
                           L4_IPC_SHORT_MSG,  /* snd desc */
                           &ignore, &ignore,  /* data */
                           L4_IPC_NEVER,      /* timeout */
                           &dope );
      if(lockdebug)
	{
	  sit_and_wait(8);
	}
      if(res)
	ipc_error("lock", res);
    }
}


/* say hello */
static void hello(char * id)
{
  l4_threadid_t my_id, my_preempter, my_pager;
  l4_umword_t esp;

  if(get_thread_ids(&my_id, &my_preempter, &my_pager, &esp))
    printf("get_thread_ids faild\n");

  printf("This is %s, task %x, thread %u, "
	 "preempter %u(%u), pager %u(%u), stack %08lx\n",
	 id,
	 my_id.id.task, my_id.id.lthread, 
	 my_preempter.id.task, my_preempter.id.lthread, 
	 my_pager.id.task, my_pager.id.lthread,
	 esp);
}


char lock_thread_stack[2048];

/* the lock thread */
static void lock_thread(void)
{
  int res;
  l4_threadid_t src;
  l4_umword_t ignore;
  l4_msgdope_t dope;

  hello("locker");

  while(1)
    {
      res = l4_ipc_wait(&src,             /* source */
                        L4_IPC_SHORT_MSG, /* rcv desc */
                        &ignore,
                        &ignore,          /* data */
                        L4_IPC_NEVER,     /* timeout */
                        &dope);           /* dope */

      if(res)
	ipc_error("ack", res);

      if(lockdebug)
	printf("\nlock acquired from %d(%d)\n", src.id.task, src.id.lthread);
	     
      res = l4_ipc_send(src,              /* dest */
                        L4_IPC_SHORT_MSG, /* snd desc */
                        ignore, ignore,   /* data */
                        L4_IPC_NEVER,     /* timeout */
                        &dope );
      if(res)
	ipc_error("lock thread", res);
      if(lockdebug)
	printf("\nlock released\n");
    }
}



/* get kernel info page */
static int get_kinfo(void)
{
  l4_threadid_t my_pager;

  get_thread_ids(NULL, NULL, & my_pager, NULL );

  kinfo = l4sigma0_kip_map(L4_INVALID_ID);

  if (!kinfo)
    {
      printf("map kip failed!\n");
      return -1;
    }

  return 0;
}



/* try to get real memory from real_min till real_max 
 * and map all pages consecutively at client_stack_base_addr
 *
 * the array reserved (of size reserved_max) contains (page aligned) 
 * regions of reserved memory. Such reserved pages are not acqired.
 */
static 
int get_memory(int reserved_max, l4_region_t * reserved)
{
  l4_threadid_t my_pager;
  l4_addr_t mem;
  l4_addr_t map = client_stack_base_addr;
  int res;
  int count = 0;

  get_thread_ids( NULL, NULL, &my_pager, NULL);

  while (1)
    {
      res = l4sigma0_map_anypage(my_pager, map, L4_LOG2_PAGESIZE, &mem);
     
      if (res==-3)
	break;
      else if (res)
	{
	  lock();
	  printf("error: %s\n", l4sigma0_map_errstr(res));
	  unlock();
	  break;
	}
      
      map += L4_PAGESIZE;
      count++;
    }

  return count;
}


/* region of acquired task numbers, get initialised in get_tasks */
static int task_min = -1;
static int task_max = -1;
#define TASK_MAX 2048

/* get all tasks */
static int get_tasks(void)
{
  int res, task;
  int got_flag = 0;
  int count = 0;

  /* use the rmgr library now */
  res = rmgr_init();
  if( res != 1){
    printf("rmgr_init failed with %x\n", res);
  }

  for(task = 0; task < TASK_MAX; task++){
    res = rmgr_get_task(task);
    if(res == 0)
      { 
	count++;
	if(got_flag == 0){
	  lock();
	  printf("got %d - ", task);
	  unlock();
	  got_flag = 1;
	  if(task_min == -1) 
	    task_min = task;
	}
      }
    else {
      if(got_flag == 1){
	lock();
	printf("%d\n", task);
	unlock();
	if(task_max == -1) 
	  task_max = task;
	got_flag = 0;
	break;
      }
    }
  }
  
  if(got_flag == 1){
    lock();
    printf("%d\n", task);
    unlock();
    if(task_max == -1) 
      task_max = TASK_MAX +1;
  }
  return count;
}


/* The base address of the stack in thread t. Note that all stacks
 * are at different places. This way all threads show up in the page 
 * table. 
 */
inline
static unsigned get_client_stack_base(l4_threadid_t t)
{
  return
    // (0x00100000 + (t.id.task << 20 | t.id.lthread << 12));
    (0x00100000 + (t.id.task << 12));
}

/* The end of the stack, needed for reagion checks in the pager */
inline
static unsigned get_client_stack_end(l4_threadid_t t)
{
  return get_client_stack_base(t) + L4_PAGESIZE;
}


/* The page that is mapped to a task for its stack */
inline
static unsigned get_client_stack_page(int task)
{
  return client_stack_base_addr + (task << L4_PAGESHIFT);
}


/* The pager for all tasks >= 6 */
static void pager_thread(void)
{
  int res, rw_page_fault;
  l4_threadid_t src;
  l4_addr_t fault_address, fault_eip;
  l4_snd_fpage_t fp = { .snd_base = 0, };
  l4_msgdope_t dope;

  lock();
  hello("pager");
  unlock();

  /* get first page fault */
  res = l4_ipc_wait(&src,			 /* source */
		    L4_IPC_SHORT_MSG,	 /* rcv desc */
		    & fault_address,
		    & fault_eip,		 /* data */
		    L4_IPC_NEVER,		 /* timeout */
		    &dope);		 /* dope */

  if(res)
    ipc_error("first PF", res);

  while(1)
    {
      rw_page_fault = fault_address & 0x02;
      fault_address &= 0xfffffffc;

      // do not lock in pager, because this gives dead locks
      //lock();
      if(debugpager)
	printf("[%s page fault in %d(%d) at %08lx (EIP %08lx) ",
	       rw_page_fault ? "write" : "read",
	       src.id.task, src.id.lthread,
	       fault_address, fault_eip);
      //unlock();

				/* in Code ? */
      if((l4_addr_t) & _stext <= fault_address 
         && ((l4_addr_t) & _etext > fault_address)
         && !rw_page_fault)
	{
	  // lock();
	  if(debugpager)
	    printf("Code\n");
	  // unlock();
	  fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE, 
			      L4_FPAGE_RO, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	}
                                /* in Data */
      else if((l4_addr_t) & _start <= fault_address 
              && ((l4_addr_t) & _end > fault_address))
        {
          if(debugpager)
            printf("Data\n");
          // unlock();
          fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE, 
                              L4_FPAGE_RW, L4_FPAGE_MAP);
          fp.snd_base = l4_trunc_page(fault_address);
        }
                                /* in Stack ? */
      else if ( (fault_address >= get_client_stack_base(src) &&
                 fault_address < get_client_stack_end(src)))
	{
	  // lock();
	  if(debugpager)
	    printf("Stack\n");
	  // unlock();
	  
	  fp.fpage = l4_fpage(get_client_stack_page(src.id.task), 
			      L4_LOG2_PAGESIZE, L4_FPAGE_RW, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	}

				/* somewhere else !! */
      else
	{
	  // lock();
	  if(!debugpager)
	    printf("[%s page fault in %d(%d) at %08lx (EIP %08lx) ",
		   rw_page_fault ? "write" : "read",
		   src.id.task, src.id.lthread,
		   fault_address, fault_eip);
	  printf("stop pager]\n");
	  // unlock();
	  wait_forever();
	}
	  
      // lock();
      if(debugpager)
	printf(" %s (%s), base : %08lx, size : %d, offset : %08lx]\n",
	       fp.fpage.fp.grant ? "grant" : "map",
	       fp.fpage.fp.write ? "WR" : "RO",
	       (l4_addr_t)fp.fpage.fp.page << L4_PAGESHIFT,
	       fp.fpage.fp.size,
	       fp.snd_base
	       );
      // unlock();

      /* send reply and wait for next page fault */
      res = l4_ipc_reply_and_wait
	(src,					 /* destination */
	 L4_IPC_SHORT_FPAGE,			 /* snd desc */
	 fp.snd_base,				 /* snd data 0 */
	 fp.fpage.fpage ,			 /* snd data 1 */
	 &src,					 /* source */
	 L4_IPC_SHORT_MSG,			 /* rcv desc */
	 &fault_address,
	 &fault_eip,
	 L4_IPC_NEVER,
	 &dope);

      if(res)
	ipc_error("PF", res);
    }
}


/* the thread that counts */
static 
void counter_thread(void)
{
  unsigned count = 0;
  int res;
  l4_threadid_t src;
  l4_umword_t ignore;
  l4_msgdope_t dope;

  lock();
  hello("counter");
  unlock();
  

  while(1){
    res = l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &ignore, &ignore,
		      L4_IPC_NEVER, &dope);
    if(res)
      ipc_error("counter", res);
    
    count++;

    if(count % 10000 == 0)
      {
	lock();
	printf("Counted %d mappings\n", count);
	// sit_and_wait(9);
	unlock();
      }
    else if(debug > 2 && count > 123000 && (count % 10 == 0))
      {
	lock();
	printf("Counted %d mappings\n", count);
	unlock();
	sit_and_wait(8);
      }
  }
}


/* count calling thread */
static
void count_me(void)
{
  int res;
  l4_msgdope_t dope;

  res = l4_ipc_send(counter_thread_id, L4_IPC_SHORT_MSG, 0,0, 
		    L4_IPC_NEVER, &dope);
  if(res)
    ipc_error("count_me", res);
}
    

/* first and last tasks in the mapping chain */
l4_threadid_t first_task = L4_INVALID_ID;
l4_threadid_t last_task = L4_INVALID_ID;

/* the initiator task */
l4_threadid_t initiator_task_id = L4_INVALID_ID;


static
void initiator_task(void)
{
  l4_threadid_t my_id, src, main_loop;
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;
  int res;
  int got_no_page = 1;
  int loop_count, mapdb_not_full;
  l4_addr_t map = ping_pong_base_addr;
  l4_umword_t ignore;

  lock();
  hello("initiator");
  unlock();

  get_thread_ids(&my_id,NULL,NULL,NULL);
  
  while(1){
    got_no_page = 1;
    /* cycle until we get a mapping */
    while(got_no_page)
      {
	res = l4_ipc_wait(&main_loop, 
		          L4_IPC_MAPMSG(map, L4_LOG2_PAGESIZE),
		          &fp.snd_base, 
		          &fp.fpage.fpage,	    // receive reg data 
		          L4_IPC_NEVER,	    // timeout 
		          &dope);		    // dope 

	if(res && res != L4_IPC_SEMAPFAILED)
	  {
	    lock();
	    ipc_error( "ini rcv", res);
	    unlock();
	  }
	
	if(res == 0 && l4_ipc_fpage_received(dope))
	  got_no_page = 0;
      }

    if(debug){
      lock();
      printf("ini task %x(%d) received page (%s, %s) at %08lx\n",
	     my_id.id.task, my_id.id.lthread,
	     fp.fpage.fp.grant ? "grant" : "map",
	     fp.fpage.fp.write ? "RW" : "RO",
	     map
	     );
      unlock();
      // sit_and_wait(9);
    }
    
    count_me();
    // resend this page for playing pingpong 
    fp.fpage.fp.page = map >> L4_PAGESHIFT;
    map += L4_PAGESIZE;
    loop_count = 1;
    mapdb_not_full = 1;
    while(mapdb_not_full){
      if (debug){
        lock();
        printf("ini task: resend page (%05lx) : %d\n",
	       (l4_addr_t)fp.fpage.fp.page, loop_count);
        unlock();
        //sit_and_wait(8);
      }
      
      res = l4_ipc_reply_and_wait(first_task, 
	   		          L4_IPC_SHORT_FPAGE,
				  fp.snd_base, fp.fpage.fpage,
				  &src,
				  L4_IPC_SHORT_MSG,
				  &ignore, &ignore,
				  L4_IPC_NEVER,
				  &dope);
      loop_count++;
      
      if(res == L4_IPC_SEMAPFAILED)
	{ // mapdb seems full, take the next page 
	  mapdb_not_full = 0;
	}
      else if(res){		/* some other error */
	lock();
	ipc_error("ini snd", res);
	unlock();
      }
    }
    /* signal main loop back */
    res = l4_ipc_send(main_loop, L4_IPC_SHORT_MSG,0,0, 
		      L4_IPC_NEVER, &dope);
    if(res){
      lock();
      ipc_error("ini signal", res);
      unlock();
    }
  }
}


static
void ping_pong_task(void)
{
  l4_taskid_t my_id, dest, src;
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;
  int res;
  int got_no_page = 1;
  l4_addr_t map = ping_pong_base_addr;

  lock();
  hello("ping");
  unlock();

  get_thread_ids(&my_id,NULL,NULL,NULL);
  
  if(my_id.id.task == last_task.id.task)
    dest = first_task;
  else
    {
      dest = my_id;
      dest.id.task += 1;
    }

  while(1){
    got_no_page = 1;
    /* cycle until we get a mapping */
    while(got_no_page)
      {
	res = l4_ipc_wait(&src, L4_IPC_MAPMSG(map, L4_LOG2_PAGESIZE),
			  &fp.snd_base, 
			  &fp.fpage.fpage,	    // receive reg data 
			  L4_IPC_NEVER,	    // timeout 
			  &dope);		    // dope 

	if(res && res != L4_IPC_REMAPFAILED)
	  {
	    lock();
	    ipc_error( "ping rcv", res);
	    unlock();
	  }
	
	if(res == 0 && l4_ipc_fpage_received(dope))
	  got_no_page = 0;
      }

    if(debug > 10 || map % 0x01000000 == 0){
      lock();
      printf("ping %d(%d) received page (%s, %s) at %08lx\n",
	     my_id.id.task, my_id.id.lthread,
	     fp.fpage.fp.grant ? "grant" : "map",
	     fp.fpage.fp.write ? "RW" : "RO",
	     map
	     );
      unlock();
      // sit_and_wait(10);
    }
    
    count_me();
    // resend the same page, increasing nesting size
    fp.fpage.fp.page = map >> L4_PAGESHIFT;
    map += L4_PAGESIZE;
    res = l4_ipc_send(dest, L4_IPC_SHORT_FPAGE,
		      fp.snd_base, fp.fpage.fpage,
		      L4_IPC_NEVER,
		      &dope);

    if(res == L4_IPC_SEMAPFAILED)
      { // ping pong mapping failed, signal the initiator
	res = l4_ipc_send(initiator_task_id, L4_IPC_SHORT_MSG,
			  0, 0,
			  L4_IPC_NEVER,
			  &dope);
      }

    if(res){
      lock();
      ipc_error("ping snd", res);
      unlock();
    }

  }
}


/* start a task, this is called in the spinner thread */
static
void start_task(int task)
{
  l4_threadid_t t, new_t;

  get_thread_ids(&t,NULL,NULL,NULL);
  t.id.task = task;
  t.id.lthread = 0;

  new_t = l4_task_new(t,			 /* dest task */
		      0,			 /* prio */
		      get_client_stack_end(t),	 /* ESP */
		      (l4_umword_t)ping_pong_task,	 /* EIP */
		      pager_thread_id );	 /* pager */
}
  

/* the thread that creates all tasks */
static 
void spinner_thread(void)
{
  l4_msgdope_t dope;
  l4_threadid_t my_id, src;
  l4_umword_t task, task_min, task_max, res;

  lock();
  hello("spinner");
  unlock();
  
  /* get task numbers to create via ipc */
  res = l4_ipc_wait( &src, L4_IPC_SHORT_MSG, &task_min, &task_max,
		      L4_IPC_NEVER, &dope);
  if(res){
    lock();
    ipc_error("obtain spinner init", res);
    unlock();
  }

  lock();
  printf("Spinner creates tasks %ld - %ld\n", task_min, task_max);
  unlock();

  if (debugwait)
    sit_and_wait(10);

  get_thread_ids(&my_id, NULL,NULL,NULL);
  first_task = my_id;
  first_task.id.task = task_min;
  first_task.id.lthread = 0;

  last_task = my_id;
  last_task.id.task = task_max -1;
  last_task.id.lthread = 0;

  for(task = task_min; task < task_max; task++)
    start_task(task);

  lock();
  printf("stop spinner\n");
  unlock();
  wait_forever();
}




int main(void)
{
  l4_threadid_t my_id, my_preempter, my_pager, new_pager;

  l4_umword_t ignore, base;
  l4_msgdope_t dope;
  l4_region_t reserved_real_mem[2];
  l4_snd_fpage_t fp;

  int tasks, pages, res, i;

  hello("pingpong");
  printf("Version " __DATE__ "\n");
  printf("Main at %08lx - %08lx\n", (l4_addr_t) &_start, (l4_addr_t) & _end);

  if (debugwait)
    sit_and_wait(8);

  /* start locker */
  get_thread_ids(&my_id,&my_preempter,&my_pager,NULL);

  lock_thread_id = my_id;
  lock_thread_id.id.lthread = LOCK_THREAD;
  my_preempter = L4_INVALID_ID;
  new_pager = my_pager;
  l4_thread_ex_regs( lock_thread_id,		 /* dest thread */
		     (l4_umword_t)lock_thread,	 /* EIP */
		     (l4_umword_t)lock_thread_stack + 2048,
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &new_pager,		 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );
  // sit_and_wait(8);

  /* map kernel info page */
  if(get_kinfo()){
    lock();
    printf("get_kinfo failed\n");
    unlock();
  }
  else{
    lock();

    /* print version string */
    printf("\nKernel info page\n");
    printf("%s\n", (char *) kinfo + ((kinfo -> offset_version_strings) * 16));

#if 0
    /* print version and real memory */
    printf("version : %08x, memory : %08lx - %08lx\n",
	   kinfo -> version, 
	   (kinfo -> main_memory).start, (kinfo -> main_memory).end);

    /* show reserved regions */
    printf("reserved:  %08lx - %08lx, %08lx - %08lx\n"
	   "semi res:  %08lx - %08lx\n"
	   "dedicated: %08lx - %08lx, %08lx - %08lx\n"
	   "           %08lx - %08lx, %08lx - %08lx\n",
	   kinfo -> reserved0.start, kinfo -> reserved0.end,
	   kinfo -> reserved1.start, kinfo -> reserved1.end,
	   kinfo -> semi_reserved.start, kinfo -> semi_reserved.end,
	   kinfo -> dedicated[0].start, kinfo -> dedicated[0].end,
	   kinfo -> dedicated[1].start, kinfo -> dedicated[1].end,
	   kinfo -> dedicated[2].start, kinfo -> dedicated[2].end,
	   kinfo -> dedicated[3].start, kinfo -> dedicated[3].end);
#endif
    unlock();
  }
    
  /* fill reserved structure, reserve pages of this program 
     and the video memory */
  reserved_real_mem[0].start = 0xa0000;
  reserved_real_mem[0].end = 0xc0000;
  reserved_real_mem[1].start = (l4_addr_t) & _start;
  reserved_real_mem[1].end = (l4_addr_t) & _end;
  
  /* round towards page boundaries */
  for(i = 0; i < 2; i++)
    {
      reserved_real_mem[i].start = l4_trunc_page(reserved_real_mem[i].start);
      reserved_real_mem[i].end = l4_round_page(reserved_real_mem[i].end);
    }

  /* get all memory */
  pages = get_memory(2, reserved_real_mem);
  lock();
  printf("Got %d pages, %ld percent\n",
	 pages, 100L);
#if 0
	 (pages * L4_PAGESIZE * 100) / 
	 (kinfo -> main_memory.end - kinfo -> main_memory.start));
#endif
  unlock();

  /* get task numbers */
  tasks = get_tasks();
  lock();
  printf("Got %d tasks\n", tasks);
  unlock();

  /* adjust task_min for initiator task */
  initiator_task_id = my_id;
  initiator_task_id.id.task = task_min;
  task_min++;

  /* start pager */
  /* As stack I use one of the first 5 pages obtained from get_memory, 
     which are never used for other tasks */
  get_thread_ids(&my_id,&my_preempter,&my_pager,NULL);

  pager_thread_id = my_id;
  pager_thread_id.id.lthread = PAGER_THREAD;
  my_preempter = L4_INVALID_ID;
  new_pager = my_pager;
  l4_thread_ex_regs( pager_thread_id,		 /* dest thread */
		     (l4_umword_t)pager_thread,	 /* EIP */
		     get_client_stack_page(2),
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &new_pager,		 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );

  /* start counter */
  counter_thread_id = my_id;
  counter_thread_id.id.lthread = COUNTER_THREAD;
  my_preempter = L4_INVALID_ID;
  new_pager = my_pager;
  l4_thread_ex_regs( counter_thread_id,		 /* dest thread */
		     (l4_umword_t)counter_thread,	 /* EIP */
		     get_client_stack_page(3),
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &new_pager,		 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );


  /* start spinner */
  spinner_thread_id = my_id;
  spinner_thread_id.id.lthread = SPINNER_THREAD;
  my_preempter = L4_INVALID_ID;
  new_pager = my_pager;
  l4_thread_ex_regs( spinner_thread_id,		 /* dest thread */
		     (l4_umword_t)spinner_thread,	 /* EIP */
		     get_client_stack_page(4),
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &new_pager,		 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );
  
  res = l4_ipc_send( spinner_thread_id, L4_IPC_SHORT_MSG,
		     task_min, task_min + number_of_tasks, 
		     L4_IPC_NEVER, &dope);
  if(res)
    ipc_error("init spinner", res);
		   
  if (debugwait)
    sit_and_wait(8);

  /* start initiator */
  initiator_task_id = 
    l4_task_new(initiator_task_id,			 /* dest task */
		0,				 /* prio */
		get_client_stack_end(initiator_task_id),    /* ESP */
		(l4_umword_t)initiator_task,	 /* EIP */
		pager_thread_id );		 /* pager */

  if (debugwait)
    sit_and_wait(8);

  /* send pages from 0x80000000 */
  base = 0x80000000;
  while(1){
    fp.snd_base = 0;
    fp.fpage = l4_fpage(base, L4_LOG2_PAGESIZE, 
			L4_FPAGE_RO, L4_FPAGE_MAP);

    res = l4_ipc_call(initiator_task_id, L4_IPC_SHORT_FPAGE,
		      fp.snd_base, fp.fpage.fpage,
		      L4_IPC_SHORT_MSG,
		      &ignore,&ignore,
		      L4_IPC_NEVER,
		      &dope);
    base += L4_PAGESIZE;
    if(res){
      ipc_error("main", res);
      break;
    }
  }

  /* wait forever */
  lock();
  printf("stop main\n");
  unlock();
  wait_forever();
  return 0;
}
