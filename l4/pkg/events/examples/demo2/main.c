/*!
 * \file   events/examples/demo2/demo2.c
 *
 * \brief  Demo for sending some events between three tasks
 *
 * this file is NOT doygen documented.
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
#include <string.h>

#include <l4/crtx/crt0.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sigma0/sigma0.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>

#include <l4/events/events.h>

char LOG_tag[9]="ev_demo2";

/* the virtual address range of this program */
extern char _end, _start, _stext, _etext;

/* debug variable, set to nonzero to get more output from the pager*/
static int debugpager = 0;

/* event_ch used for this demo */
#define DEMO2_EVENTID	9

/* priority used for this demo */
#define DEMO2_PRIORITY	0

/* region of acquired task numbers, get initialised in get_tasks */
#define TASK_MAX 3
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

/* nicely report ipc errors */
static void
ipc_error(char * msg, int error)
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
static void
sit_and_wait(int exp)
{
  l4_ipc_sleep(l4_ipc_timeout(255,16-exp,255,16-exp));
}


/* shut down this thread */
static void
wait_forever(void)
{
  l4_ipc_sleep(L4_IPC_NEVER);
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

  while (count < TASK_MAX)
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

  if (debugpager)
   printf("unshared data page %08lx => %08lx client "l4util_idfmt"\n",
      addr, new_page, l4util_idstr(tid));

  if (debugpager)
   printf("copied pages: %d\n", ++copied_pages);

  return new_page;
}


/* The pager for our new tasks */
static void
pager_thread(void)
{
  int res, rw_page_fault;
  l4_threadid_t src;
  l4_addr_t fault_address, fault_eip;
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;

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
      if(debugpager)
	printf("[%s page fault in "l4util_idfmt" at %08lx (EIP %08lx) ",
	       rw_page_fault ? "write" : "read",
	       l4util_idstr(src), fault_address, fault_eip);
				/* in Code */
      if(    (l4_addr_t) & _stext <= fault_address
	 && ((l4_addr_t) & _etext > fault_address)
	 && !rw_page_fault)
	{
	  if(debugpager)
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
	  if(debugpager)
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
           if(!debugpager)
	     printf("[%s page fault in "l4util_idfmt" at %08lx (EIP %08lx) ",
		     rw_page_fault ? "write" : "read",
		     l4util_idstr(src), fault_address, fault_eip);
	    printf("stop pager]\n");
	    enter_kdebug("stop");
	    fp.snd_base = 0;
	    l4_ipc_sleep(L4_IPC_NEVER);
	}

      if(debugpager)
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


static void
print_event(l4events_ch_t event_ch, l4events_event_t *event)
{
  printf("event_ch: %d\n", event_ch);
  printf("event:   ");
  printf(event->str);
  printf("\n");

  return;
}

static void
otask1(void)
{
  l4events_ch_t event_ch = L4EVENTS_NO_CHANNEL;
  l4events_nr_t event_nr = L4EVENTS_NO_NR;
  l4events_event_t event;
  int res;

  //event.str = buffer2;

  strcpy(LOG_tag, "task1");

  printf("start.\n");

  printf("register.\n");
  l4events_register(DEMO2_EVENTID, DEMO2_PRIORITY);

  printf("receive first try.\n");
  res = l4events_receive(&event_ch, &event, &event_nr,
      			l4_ipc_timeout(0,0,1,0), 0);

  if (res == L4EVENTS_OK)
  {
    printf("receive event successful!\n");

    print_event(event_ch, &event);
  }
  else
  {
    printf("receive event failed!\n");
  }

  sit_and_wait(7);

  printf("receive second try.\n");
  event_ch = L4EVENTS_NO_CHANNEL;
  res = l4events_receive(&event_ch, &event, &event_nr,
      			L4_IPC_NEVER, 0);

  if (res == L4EVENTS_OK)
  {
    printf("receive event successful!\n");
    print_event(event_ch, &event);
  }
  else
  {
    printf("receive event failed!\n");
  }

  printf("unregister.\n");
  l4events_unregister(DEMO2_EVENTID);

  printf("stop.\n");

  wait_forever();
}


static void
otask2(void)
{
  l4events_event_t e;
  l4events_nr_t nr;

  strcpy(e.str, "ein kleiner text.");
  e.len = strlen(e.str)+1;

  strcpy(LOG_tag, "task2");

  printf("start.\n");

  sit_and_wait(8);

  printf("send event.\n");

  l4events_send(DEMO2_EVENTID, &e, &nr, 0);

  printf("stop.\n");

  wait_forever();
}

/* callback function for waiting */
static void
callback(l4events_ch_t event_ch, l4events_event_t *event)
{
  print_event(event_ch, event);
}

static void
otask3(void)
{
  strcpy(LOG_tag, "task3");

  printf("start.\n");

  l4events_register(DEMO2_EVENTID, DEMO2_PRIORITY);
  l4events_wait(1, L4EVENTS_NO_CHANNEL, callback);

  printf("end...\n");

  wait_forever();
}


// save data segment so we are able to copy-on-write it for new childs */
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

// reserve pool for copy-on-write
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
      		   (l4_addr_t)&_stext, (l4_addr_t)&_stext, &_etext-&_stext);
  l4sigma0_map_mem(pager,
      		   cow_pool, cow_pool, COW_POOL_PAGES*L4_PAGESIZE);
  cow_pages = COW_POOL_PAGES;
}


int
main(void)
{
  l4_threadid_t my_id, my_preempter, my_pager, new_pager;

  l4_umword_t ignore;
  l4_threadid_t t, new_t;

  int tasks;

  rmgr_init();

  l4events_init();

  /* this call initialized the names library
     -- must be done before save_data()! */
  names_register("eventdemo");
  
  get_thread_ids(&my_id,&my_preempter,&my_pager,NULL);

  /* call this at the very beginning! */
  save_data(my_pager);
  reserve_cow(my_pager);

  printf("starting event server demo...\n");

  /* get task numbers */
  tasks = get_tasks();

    if (tasks < TASK_MAX)
  {
    printf("got not enough tasks!!!");
    enter_kdebug("main");
  }

  /* start pager */
  get_thread_ids(&my_id,&my_preempter,&my_pager,NULL);
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


  get_thread_ids(&t,NULL,NULL,NULL);

  t.id.task = task_min;
  t.id.lthread = 0;

  printf("start task1\n");

  new_t = l4_task_new(t,			 /* dest task */
		      255,			 /* prio */
		      (l4_umword_t)&crt0_stack_high,    	 /* ESP */
		      (l4_umword_t)otask1,	 /* EIP */
		      pager_thread_id );	 /* pager */

  t.id.task = task_min+1;
  t.id.lthread = 0;

  printf("start task2\n");

  new_t = l4_task_new(t,			 /* dest task */
		      255,			 /* prio */
		      (l4_umword_t)&crt0_stack_high,     /* ESP */
		      (l4_umword_t)otask2,		 /* EIP */
		      pager_thread_id );	 /* pager */


  t.id.task = task_min+2;
  t.id.lthread = 0;

  printf("start task3\n");

  new_t = l4_task_new(t,			 /* dest task */
		      255,			 /* prio */
		      (l4_umword_t)&crt0_stack_high,	 /* ESP */
		      (l4_umword_t)otask3,		 /* EIP */
		      pager_thread_id );	 /* pager */


  /* wait forever */
  l4_ipc_sleep(L4_IPC_NEVER);

  return 0;
}

