/*!\
 * \file   events/demo3/demo3.c
 *
 * \brief  demo for sending bulks of events from many tasks to many tasks
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <l4/crtx/crt0.h>
#include <l4/events/events.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sigma0/sigma0.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/kernel.h>
#include <l4/util/rdtsc.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>

char LOG_tag[9]="ev_demo3";


/* the virtual address range of this program */
extern char _end, _stext, _etext, __bss_start;

/* debug variables, set to nonzero to get more output */
static const int debug_pager = 0;	/* debug page faults */
static const int debug_send = 0;	/* debug send operation */
static const int debug_recv = 0;	/* debug receive operation */

/*! \brief the number of sending tasks */
#define SEND_TASK_COUNT		1
/*! \brief the number of receiving tasks */
#define RECV_TASK_COUNT		1
/* total number of required tasks */
#define TASK_COUNT		(SEND_TASK_COUNT + RECV_TASK_COUNT)
/*! \brief this is the number of events to sent per task */
#define SEND_EVENT_COUNT	100
/* this is the number of events received per task */
#define RECV_EVENT_COUNT	(SEND_EVENT_COUNT * SEND_TASK_COUNT)

/* time benchmark, set to nonzero to enable time measurement */
static const int timebenchmark = 1;

/*! \brief set this constant to the correct value for your local machine !!! */
#define PINGPONG_IPC	5000

/* event_ch used for this demo */
#define DEMO3_EVENTID		10
/* priority used for this demo */
#define DEMO3_PRIORITY		0

/* region of acquired task numbers, get initialised in get_tasks */
static int task_min = -1;
static int task_max = -1;

/* pager thread variables */
#define PAGER_THREAD 1	/* thread number definition */
static l4_threadid_t pager_thread_id = L4_INVALID_ID; /* thread id */
char pager_thread_stack[2048]; /* stack */


/* varaiables for COW mechanism */
#define COW_POOL_PAGES    2000
static int copied_pages = 0;
static l4_addr_t cow_pool;
static l4_addr_t cow_pages;
static l4_addr_t my_program_data_copy;

static l4_addr_t unshare_data(l4_addr_t addr, l4_threadid_t tid);

/* data structure for communication, should not be unshared*/
typedef volatile struct
{
  l4_uint32_t   start_tsc;	/* start time for benchmark */
  l4_uint32_t	nr_pagefaults;	/* pagefault counter */
  int           recv_task_min;	/* minimal recv_task_id */
  int		send_task_min;	/* minimal send_task_id */
  l4_threadid_t main_thread_id;	/* main thread_id */
} shared_data_t;

/* shared datas */
static shared_data_t *shared_data = 0;


/* nicely report ipc errors */
static
void ipc_error(char * msg, int error)
{
  if(error == 0){
    printf("%s : ok\n", msg);
  }
  else {
    printf("  \n");

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

/* get thread number, preempter, pager, and ESP of calling thread */
static int
get_thread_ids(l4_threadid_t *my_id,
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

/* get the apropriate number of task ids */
static int
get_tasks(void)
{
  int res, task;
  int got_flag = 0;
  int count = 0;

  /* use the rmgr library now */
  res = rmgr_init();

  if( res != 1){
    printf("rmgr_init failed with %x\n", res);
  }

  task = 0;

  while (count < TASK_COUNT)
  {
    res = rmgr_get_task(task);

    if(res == 0)
    {
	count++;
	if(got_flag == 0)
	{
	  got_flag = 1;
	  if(task_min == -1) task_min = task;
	}
    }
    else
    {
      if(got_flag == 1)
      {
	got_flag = 0;
	if(task_max == -1) task_max = task;
	break;
      }
    }

    task++;
  }

  return count;
}



/* copy-on-write mechanism: allocate a new page and copy appropriate content
 of data into new page */
static l4_addr_t
unshare_data(l4_addr_t addr, l4_threadid_t tid)
{
  l4_addr_t new_page = cow_pool;
  l4_addr_t img_base = l4_trunc_page(addr);
  l4_addr_t data_start = (l4_addr_t)&_etext;
  l4_addr_t data_end   = (l4_addr_t)&_end;

  if (!cow_pages)
    {
      printf("PANIC: Out of cow pages\n");
      exit(-1);
    }

  cow_pool += L4_PAGESIZE;
  cow_pages--;

  memcpy((void*)new_page, (void*)img_base, L4_PAGESIZE);

  /* bss is located at &__bss_start ... &_end-1 */
  if (!(   (img_base+L4_PAGESIZE <= data_start)
        || (img_base             >= data_end)))
    {
      /* some space of the page we want to give out
       * is bss -- clear it! */
      l4_addr_t dst_offs;
      l4_addr_t src_offs;
      l4_size_t src_size;
      if (data_start <= img_base)
	{
	  src_offs = img_base - data_start;
	  dst_offs = 0;
	  if (data_end >= img_base + L4_PAGESIZE)
       	    src_size = L4_PAGESIZE;
	  else
	    src_size = data_end-img_base;
	}
      else
	{
	  src_offs = 0;
	  dst_offs = data_start - img_base;
	  if (data_end >= img_base + L4_PAGESIZE)
	    src_size = L4_PAGESIZE-dst_offs;
	  else
	    src_size = data_end-img_base-dst_offs;
	}

      memcpy((void*)new_page+dst_offs,
	     (void*)my_program_data_copy+src_offs,
	     src_size);
#if 0
      printf("offs_shared_data=%08x\n"
	     "copy from %08x-%08x to %08x-%08x\n"
	     "data from %08x-%08x page %08x-%08x\n"
	     "saved_data %08x-%08x\n",
	     (l4_umword_t)&shared_data - (l4_umword_t)&_etext,
	     my_program_data_copy+src_offs,
	     my_program_data_copy+src_offs+src_size,
	     new_page+dst_offs,
	     new_page+dst_offs+src_size,
	     data_start, data_end,
	     img_base, img_base+L4_PAGESIZE,
	     my_program_data_copy, my_program_data_copy+data_end-data_start);
      enter_kdebug("stop");
#endif
    }

  if (debug_pager)
   printf("unshared data page %08lx => %08lx client "l4util_idfmt"\n",
      addr, new_page, l4util_idstr(tid));

  if (debug_pager)
   printf("copied pages: %d\n", ++copied_pages);

  return new_page;
}


/*! \brief the pager for our new tasks */
static void
pager_thread(void)
{
  int res, rw_page_fault;
  l4_threadid_t src;
  l4_addr_t fault_address, fault_eip;
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;

  l4_touch_ro((void*)&_stext, l4_round_page(&_etext - &_stext));
  l4_touch_rw((void*)shared_data, L4_PAGESIZE);

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

      shared_data->nr_pagefaults++;

      // do not lock in pager, because this gives dead locks
      if(debug_pager)
	printf("[%s page fault in "l4util_idfmt" at %08lx (EIP %08lx) ",
	       rw_page_fault ? "write" : "read",
	       l4util_idstr(src), fault_address, fault_eip);
				/* in Code */
      if(    (l4_addr_t) & _stext <= fault_address
	 && ((l4_addr_t) & _etext > fault_address)
	 && !rw_page_fault)
	{
	  if(debug_pager)
	    printf("Code\n");
	  fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE,
			      L4_FPAGE_RO, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	}
				/* in Data/Bss? */
      else if((l4_addr_t) & _etext <= fault_address
	 &&  ((l4_addr_t) & _end   > fault_address+3))
	{
	  l4_addr_t new_page;
	  if(debug_pager)
	   printf("Data\n");

	  if (rw_page_fault)
	    {
	      new_page = unshare_data(fault_address, src);
	      fp.fpage = l4_fpage(new_page, L4_LOG2_PAGESIZE,
			          L4_FPAGE_RW, L4_FPAGE_MAP);
	      l4_fpage_unmap(l4_fpage(l4_trunc_page(fault_address),
		                      L4_LOG2_PAGESIZE, 0, 0),
		                      L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
	    }
	  else
	    {
	      new_page = l4_trunc_page(fault_address);
	      fp.fpage = l4_fpage(new_page, L4_LOG2_PAGESIZE,
			          L4_FPAGE_RO, L4_FPAGE_MAP);
	    }

	  fp.snd_base = l4_trunc_page(fault_address);
	}

				/* in Stack ? */
      else
	{
	  if (    ((l4_addr_t)shared_data <= fault_address)
	      && (((l4_addr_t)shared_data + L4_PAGESIZE) > fault_address))
	  {
	    if(debug_pager)
	      printf("Shared data\n");
	    fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE,
		        L4_FPAGE_RO, L4_FPAGE_MAP);
	    fp.snd_base = l4_trunc_page(fault_address);
	  }
	  else
	  {

	    if(!debug_pager)
	     printf("[%s page fault in "l4util_idfmt" at %08lx (EIP %08lx) ",
		     rw_page_fault ? "write" : "read",
		     l4util_idstr(src), fault_address, fault_eip);
	    printf("stop pager]\n");
	    enter_kdebug("stop");
	    fp.snd_base = 0;
	    l4_ipc_sleep(L4_IPC_NEVER);
	  }
	}

      if(debug_pager)
	printf(" %s (%s), base : %08x, size : %d, offset : %08lx]\n",
	       fp.fpage.fp.grant ? "grant" : "map",
	       fp.fpage.fp.write ? "WR" : "RO",
	       fp.fpage.fp.page << L4_PAGESHIFT,
	       fp.fpage.fp.size,
	       fp.snd_base
	       );

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

/*! \brief function implements the receiving task */
static void
recv_task(void)
{
  l4_taskid_t my_id;
  l4_umword_t ignore;
  l4_msgdope_t dope;

  l4events_ch_t event_ch = DEMO3_EVENTID;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;
  int i, res;

  l4_uint32_t in ,out;
  l4_uint32_t ipc_start, ipc_end;
  static l4_uint32_t all_ipc_times[RECV_EVENT_COUNT]; // this must be static!!!
  int ipc_counter = 0;

  char my_name[16];

  my_id = l4_myself();

  sprintf(my_name, "rcv %3d", my_id.id.task-shared_data->recv_task_min);
  strcpy(LOG_tag, my_name);
  
  /* make sure that we don't get any page faults on code and stack
     sections and shared pages while benchmarking */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&crt0_stack_low, &crt0_stack_high-&crt0_stack_low);

  printf("started.\n");

  l4events_register(DEMO3_EVENTID, DEMO3_PRIORITY);

  printf("registered.\n");

  /* give 'feedback' to main task */
  l4_ipc_send(shared_data->main_thread_id, 0,
	      (l4_umword_t)0, (l4_umword_t)0, L4_IPC_NEVER, &dope);

  /* wait for main task to start */
  l4_ipc_receive(shared_data->main_thread_id,
		 0, &ignore, &ignore, L4_IPC_NEVER, &dope);


  /* loop for receiving events */
  for (i = 0; i < RECV_EVENT_COUNT; i++)
  {
    ipc_start = l4_rdtsc_32();
    res = l4events_receive(&event_ch, &event, &event_nr, 
			L4_IPC_NEVER, L4EVENTS_RECV_SHORT);
    ipc_end = l4_rdtsc_32();

    all_ipc_times[ipc_counter++] = (ipc_end-ipc_start);

    if (res != L4EVENTS_OK)
    {
       printf("recv failed with %d.\n", res);
    }
  }

  out = l4_rdtsc_32();
  in = shared_data->start_tsc;

  /* make sure, only the first task reports benchmark result */
  if (timebenchmark && (my_id.id.task == shared_data->recv_task_min))
  {
    if (shared_data->nr_pagefaults != 0)
    {
      printf("WARNING: %d pagefaults occured during measurment!\n",
	     shared_data->nr_pagefaults);
    }

    printf("number of CPU cycles: %u\n", (l4_uint32_t)(out-in));
    printf("number of IPC calls:  %d\n", ipc_counter);
    printf("PINGPONG ratio:       %d\n\n", (l4_uint32_t)((out-in)/(PINGPONG_IPC*ipc_counter)));

    for (i=0; i < ipc_counter; i++)
    {
      printf("time for ipc %u is %u \n", i, all_ipc_times[i]);
    }
  }

  l4events_unregister(DEMO3_EVENTID);

  printf("end.\n");

  /* wait forever */
  l4_ipc_sleep(L4_IPC_NEVER);

  return;
}

/*! \brief function implements the sending task */
static void
send_task(void)
{
  l4_taskid_t my_id;
  l4_msgdope_t dope;
  l4events_event_t event;
  l4events_nr_t event_nr;
  int i, res;
  char my_name[16];

  my_id = l4_myself();

  sprintf(my_name, "rcv %3d", my_id.id.task-shared_data->recv_task_min);
  strcpy(LOG_tag, my_name);

  /* make sure that we don't get any page faults on code and stack sections
     while benchmarking */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_ro((void*)shared_data, sizeof(shared_data_t));
  l4_touch_rw(&crt0_stack_low, &crt0_stack_high-&crt0_stack_low);

  /* prepare a very short event */
  strcpy(event.str, "hi");
  event.len = strlen(event.str)+1;

  /* send loop */
  for (i = 0; i < SEND_EVENT_COUNT; i++)
  {
    if(debug_send)
    {
      printf("%d sending\n", my_id.id.task);
    }

    res = l4events_send(DEMO3_EVENTID, &event, &event_nr, 0);

    if(debug_send)
    {
      if (res == L4EVENTS_OK)
      {
        printf("%d send successful.\n", my_id.id.task);
      }
      else
      {
        printf("%d send failed.\n", my_id.id.task);
      }
    }
  }

  /* giving feedback to the main task
     that receive tasks can be started */
  l4_ipc_send(shared_data->main_thread_id,
	      0, (l4_umword_t)0, (l4_umword_t)0, L4_IPC_NEVER, &dope);


  /* wait forever */
  l4_ipc_sleep(L4_IPC_NEVER);

  return;
}

/* these are some utility functions for memeory management */

/*! \brief grabs some shared memory */
static void
get_shared_memory(l4_threadid_t pager)
{
  assert(sizeof(shared_data_t) <= L4_PAGESIZE);

  shared_data = (shared_data_t*)rmgr_reserve_mem(L4_PAGESIZE,
						 L4_LOG2_PAGESIZE, 0, 0, 0);
  l4sigma0_map_mem(pager,
      		  (l4_addr_t)shared_data, (l4_addr_t)shared_data, L4_PAGESIZE);

  if (shared_data == (shared_data_t*)~0U)
  {
    printf("Cannot allocate page for sharing data section\n");
    enter_kdebug("out of memory");
  }
}

/*! \brief save data segment so we are able to copy-on-write it for childs */
static void
save_data(l4_threadid_t pager)
{
  l4_addr_t beg  = l4_trunc_page(&_etext);
  l4_addr_t end  = l4_round_page(&_end);

  my_program_data_copy = rmgr_reserve_mem(end-beg, L4_LOG2_PAGESIZE, 0, 0, 0);

  if (my_program_data_copy == ~0U)
  {
    printf("Cannot allocate %ldkB memory for unsharing data section\n",
	  (end-beg)/1024);
    enter_kdebug("out of memory");
  }

  l4sigma0_map_mem(pager,
      		   my_program_data_copy, my_program_data_copy, end-beg);
  memcpy((void*)my_program_data_copy, (void*)&_etext, &_end-&_etext);
}

/*! \brief reserve pool for copy-on-write */
static void
reserve_cow(l4_threadid_t pager)
{
  cow_pool = rmgr_reserve_mem(COW_POOL_PAGES*L4_PAGESIZE,
                              L4_LOG2_PAGESIZE, 0, 0, 0);
  if (cow_pool == ~0U)
    {
      printf("Cannot allocate %dkB memory for cow pool!\n",
	  COW_POOL_PAGES*L4_PAGESIZE/1024);
      enter_kdebug("out of memory");
    }

  l4sigma0_map_mem(pager,
      		   cow_pool, cow_pool, COW_POOL_PAGES*L4_PAGESIZE);
  cow_pages = COW_POOL_PAGES;
}


int main(void)
{
  l4_threadid_t my_id, my_preempter, my_pager, new_pager;

  l4_umword_t ignore;
  l4_msgdope_t dope;
  l4_threadid_t t, new_t;

  int task_curr, tasks, i;

  /* initialize the resource manager */
  rmgr_init();

  /* initialize the events server */
  l4events_init();

  /* initialize the the names library
     -- must be done before save_data()! */
  names_register("eventdemo");

  get_thread_ids(&my_id, &my_preempter, &my_pager, NULL);
  
  /* initialize the memory management system */
  get_shared_memory(my_pager);
  save_data(my_pager);
  reserve_cow(my_pager);

  printf("starting event server demo...\n");

  /* get self task id */
  t = l4_myself();

  /* get the task apropriate number of tasks */
  tasks = get_tasks();

  printf("got %d tasks.\n", tasks);

  if (tasks < TASK_COUNT)
  {
    printf("got not enough tasks!!!");
    enter_kdebug("main");
  }

  /* start pager thread */
  shared_data->main_thread_id = my_id;
  pager_thread_id = my_id;
  pager_thread_id.id.lthread = PAGER_THREAD;
  my_preempter = L4_INVALID_ID;
  new_pager = my_pager;
  l4_thread_ex_regs( pager_thread_id,		 /* dest thread */
		     (l4_umword_t)pager_thread,	 /* EIP */
		     (l4_umword_t)pager_thread_stack + 2048,
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &new_pager,		 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );

  task_curr = task_min;
  shared_data->recv_task_min = task_min;
  shared_data->send_task_min = task_min+RECV_TASK_COUNT;

  /* start receive tasks */
  for (i=0; i < RECV_TASK_COUNT; i++)
  {
    t.id.task = task_curr;
    t.id.lthread = 0;

    printf("start recv task.\n");

    new_t = l4_task_new(t,
		        155,
		        (l4_umword_t)&crt0_stack_high,
		        (l4_umword_t)recv_task,
		        pager_thread_id );

    task_curr = task_curr + 1;
  }

  printf("waiting for receive tasks to become ready.\n");

  for (i=0; i<RECV_TASK_COUNT; i++)
  {
    t.id.task = shared_data->recv_task_min+i;
    t.id.lthread = 0;

    l4_ipc_receive(t, 0, &ignore, &ignore, L4_IPC_NEVER, &dope);
  }

  /* start send tasks */
  for (i=0; i<SEND_TASK_COUNT; i++)
  {
    t.id.task = task_curr;
    t.id.lthread = 0;

    printf("start send task.\n");

    new_t = l4_task_new(t,
		        255,
		        (l4_umword_t)&crt0_stack_high,
		        (l4_umword_t)send_task,
		        pager_thread_id );

    task_curr = task_curr + 1;
  }

  printf("waiting for send tasks to become ready.\n");

  for (i=0; i<SEND_TASK_COUNT; i++)
  {
    t.id.task = shared_data->send_task_min+i;
    t.id.lthread = 0;
    l4_ipc_receive(t, 0, &ignore, &ignore, L4_IPC_NEVER, &dope);
  }

  /* reset pagefault counter */
  shared_data->nr_pagefaults = 0;
  /* set the start time */
  shared_data->start_tsc = l4_rdtsc_32();

  printf("waking up receive tasks.\n");

  for (i=0; i<RECV_TASK_COUNT; i++)
  {
    t.id.task = shared_data->recv_task_min+i;
    t.id.lthread = 0;
    l4_ipc_send(t, 0, (l4_umword_t)0, (l4_umword_t)0, L4_IPC_NEVER, &dope);
  }

  printf("end of the game.\n");

  /* wait forever */
  l4_ipc_sleep(L4_IPC_NEVER);

  return 0;
}

