/* $Id$ */
/*****************************************************************************
 * simple_ts/examples/simple_ts_test/main.c                                  *
 *                                                                           *
 * Created:   08/29/2000                                                     *
 * Author(s): Frank Mehnert <fm3@os.inf.tu-dresden.de>                       *
 *                                                                           *
 * Common L4 environment                                                     *
 * Example for using the task server                                         *
 *****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/generic_ts/generic_ts-client.h>

/* other includes */
#include <stdio.h>

typedef struct {
    l4dm_dataspace_t	ds;
    l4_addr_t		start;
    l4_addr_t		end;
} stack_t;

#define MAX_TASK_CNT	1024
#define MAX_TASK_ID	1024

static stack_t pager_stack;
static stack_t app_stack[MAX_TASK_CNT];
static int task2app[MAX_TASK_ID] = {0, };

static void
app(int number, l4_threadid_t caller)
{
  l4_umword_t dw1, dw2;
  l4_msgdope_t result;
  l4_threadid_t src_id;
  l4_umword_t dummy;

  printf("--> App %d: Hello World!\n", number);
  l4_i386_ipc_call(caller, 
      		   L4_IPC_SHORT_MSG, 0, 0, 
      		   L4_IPC_SHORT_MSG, &dummy, &dummy,
		   L4_IPC_NEVER, &result);
  for (;;)
    {
      /* receive anything from anywhere but don't answer */
      l4_i386_ipc_wait(&src_id, L4_IPC_SHORT_MSG, &dw1, &dw2,
	  	       L4_IPC_NEVER, &result);
    }
}


static void
app_pager(l4_threadid_t caller)
{
  extern char _end;
  l4_umword_t dw1, dw2, dummy;
  void *reply_type;
  l4_msgdope_t result;
  l4_threadid_t src_thread;

  l4_i386_ipc_call(caller,
      		   L4_IPC_SHORT_MSG, 0, 0,
		   L4_IPC_SHORT_MSG, &dummy, &dummy,
		   L4_IPC_NEVER, &result);
  for (;;)
    {
      int error = l4_i386_ipc_wait(&src_thread,
	                           L4_IPC_SHORT_MSG, &dw1, &dw2,
				   L4_IPC_NEVER, &result);
      while (!error)
	{
	  int appidx;

	  if (src_thread.id.task > MAX_TASK_ID)
	    {
	      printf("pagefault at %08x (eip %08x) from unknown task %x.%x\n",
		     dw1, dw2, src_thread.id.task, src_thread.id.lthread);
	      enter_kdebug("stop");
	      continue;
	    }

	  appidx = task2app[src_thread.id.task];
	  reply_type = L4_IPC_SHORT_FPAGE;
	  if ((dw1 == 1) && ((dw2 & 0xff) == 1))
	    {
	      printf("kernel info page requested, giving up ...\n");
	      exit(-1);
	    }
	  else if (dw1 >= 0x40000000)
	    {
	      printf("adapter pages requested, giving up ...\n");
	      exit(-1);
	    }
	  else if ((dw1 & 0xfffffffc) == 0)
	    {
	      printf("null pointer exception thread %x.%x, (%08x at %08x)\n",
		     src_thread.id.task, src_thread.id.lthread, dw1, dw2);
	      enter_kdebug("stop");
	    }
	  else if ((dw1 >= (l4_umword_t)&app) &&
		   (dw1  < (l4_umword_t)&task2app))
	    {
	      /* pf in text section (a bit tricky, normally we should
	       * have knowledge about the ELF file sections) */
	      dw1 &= L4_PAGEMASK;
	      dw2 = l4_fpage(dw1, L4_LOG2_PAGESIZE, 
		                  L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
	    }
	  else if ((dw1 >= (l4_umword_t)&task2app) &&
	           (dw1 < (l4_umword_t)&_end))
	    {
	      /* pf in data section (remarks like text section) */
	      dw1 &= L4_PAGEMASK;
	      dw2 = l4_fpage(dw1, L4_LOG2_PAGESIZE,
		                  L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
	    }
	  else if ((dw1 >= app_stack[appidx].start) &&
	           (dw1 <  app_stack[appidx].end))
	    {
	      dw1 &= L4_PAGEMASK;
	      dw2 = l4_fpage(dw1, L4_LOG2_PAGESIZE,
		  		  L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
	    }
	  else
	    {
	      printf("unknown pagefault at %08x (eip %08x) from %x.%x\n"
		     "eip = %08x..%08x  stack = %08x..%08x\n",
		      dw1, dw2, src_thread.id.task, src_thread.id.lthread,
		      (unsigned)&app, (unsigned)&task2app,
		      app_stack[appidx].start, app_stack[appidx].end);
	      enter_kdebug("stop");
	    }

	  error = l4_i386_ipc_reply_and_wait(src_thread, reply_type, dw1, dw2,
	      				     &src_thread, L4_IPC_SHORT_MSG,
					     &dw1, &dw2, 
					     L4_IPC_TIMEOUT(0, 1, 0, 0, 0, 0),
					     &result);
	}

      printf("pager: IPC error %x\n", error);
    }
}

int
main(void)
{
  int error, i;
  l4_threadid_t ts_id;
  l4_threadid_t tid, pager;
  l4_threadid_t my_preempter, my_pager;
  l4_threadid_t ext_preempter, partner;
  sm_exc_t exc;
  unsigned int *esp;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4_sched_param_t sched;

  LOG_init("s_tstst");

  if (!names_waitfor_name("SIMPLE_TS", &ts_id, 5000))
    {
      printf("SIMPLE_TS not found\n");
      return -1;
    }

  /* allocate pager stack */
  pager_stack.start = (l4_addr_t)l4dm_mem_ds_allocate(L4_PAGESIZE,0,
						      &pager_stack.ds);
  if (pager_stack.start == 0)
    {
      printf("allocate pager stack failed");
      exit(-1);
    }

  /* get thread parameters */
  my_preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(),
      		    (l4_umword_t)-1, (l4_umword_t)-1,
		    &my_preempter, &my_pager,
		    &dummy, &dummy, &dummy);
  ext_preempter = partner = L4_INVALID_ID;
  l4_thread_schedule(l4_myself(),
                     L4_INVALID_SCHED_PARAM,
		     &ext_preempter, &partner,
		     &sched);

  /* set our priority to 20 */
  sched.sp.prio = 20;
  ext_preempter = partner = L4_INVALID_ID;
  l4_thread_schedule(l4_myself(), sched, &ext_preempter, &partner, &sched);

  /* start pager thread */
  pager = l4_myself();
  pager.id.lthread = 3;

  /* set stack pointer, touch page */
  esp = (unsigned int*)(pager_stack.start + L4_PAGESIZE);
  *(--((l4_threadid_t*)esp)) = l4_myself();
  *(--esp) = 0;

  /* create pager thread */
  l4_thread_ex_regs(pager,
      		    (l4_umword_t)app_pager, (l4_umword_t)esp,
		    &my_preempter, &my_pager,
		    &dummy, &dummy, &dummy);

  /* wait for pager up */
  l4_i386_ipc_receive(pager, 0, &dummy, &dummy, L4_IPC_NEVER, &result);
  l4_i386_ipc_send(pager, 0, 0, 0, L4_IPC_NEVER, &result);

  printf("Pager is up.\n");

  for (i=0;i<MAX_TASK_CNT;i++)
    {
      /* allocate one page as stack for new task */
      app_stack[i].start = (l4_addr_t)l4dm_mem_ds_allocate(L4_PAGESIZE, 0,
							   &app_stack[i].ds);
      if (app_stack[i].start == 0)
	{
	  printf("error allocating memory for stack");
	  exit(-1);
	}

      app_stack[i].end = app_stack[i].start + L4_PAGESIZE - 1;
      esp = (unsigned int*)(app_stack[i].start+L4_PAGESIZE);

      /* put app number and myself on top of stack as parameter */
      *(--((l4_threadid_t*)esp)) = l4_myself();
      *(--esp) = i;
      *(--esp) = 0;

      /* Allocate app number i. We can't allocate and create the task in
       * one step because after creating the task runs immediatly so it
       * may be that the new task's pager doesn't know anything about the
       * new task (because the task already runs before the task create to
       * the task server returns */
      error = l4_ts_allocate(ts_id, (l4_ts_taskid_t*)&tid, &exc);
      if (error || exc._type != exc_l4_no_exception)
	{
	  /* most probably we have reached the maximum number of tasks */
	  printf("Expected error allocating task: %d\n", error);
	  break;
	}

      if (tid.id.task > MAX_TASK_ID)
	{
	  printf("task number is %d (greater than %d)\n",
	        tid.id.task, MAX_TASK_ID);
	  return -1;
	}
      task2app[tid.id.task] = i;

      /* Create task. We choose a priority of 19 and an mcp of 0xe0. */
      error = l4_ts_create(ts_id, (l4_ts_taskid_t*)&tid, 
			   (l4_umword_t)app, (l4_umword_t)esp, 0xe0,
	                   (l4_ts_taskid_t*)&pager, 21, "",  0, &exc);
      if (error || exc._type != exc_l4_no_exception)
	{
	  printf("Error %d creating task\n", error);
	  break;
	}
      
      /* shake hands with new app */
      l4_i386_ipc_receive(tid, 0, &dummy, &dummy, L4_IPC_NEVER, &result);
      l4_i386_ipc_send(tid, 0, 0, 0, L4_IPC_NEVER, &result);

      printf("Task %x.%x (%08x:%08x) stack at %08x..%08x is up\n",
	     tid.id.task,tid.id.lthread,tid.lh.high,tid.lh.low,
	     (unsigned)esp & L4_PAGEMASK, 
	     ((unsigned)esp & L4_PAGEMASK)+L4_PAGESIZE-1);
    }

  /* give summary */
  printf("%d tasks created\n", i);

  error = l4_ts_delete(ts_id, (l4_ts_taskid_t*)&tid, &exc);
  if (error)
    {
      printf("error deleting task %x.%x\n", 
	     tid.id.task, tid.id.lthread);
      return -1;
    }

  printf("Task %x.%x killed\n", tid.id.task, tid.id.lthread);

  error = l4_ts_free(ts_id, (l4_ts_taskid_t*)&tid, &exc);
  if (error || exc._type != exc_l4_no_exception)
    {
      printf("Error %d freeing task %x.%x\n", 
	      error, tid.id.task, tid.id.lthread);
      return -1;
    }
  
  printf("Task %x.%x freed\n", tid.id.task, tid.id.lthread);

  error = l4_ts_allocate(ts_id, (l4_ts_taskid_t*)&tid, &exc);
  if (error || exc._type != exc_l4_no_exception)
    {
      printf("Error %d allocating task %x.%x\n", 
	      error, tid.id.task, tid.id.lthread);
      return -1;
    }

  if (tid.id.task > MAX_TASK_ID)
    {
      printf("task number is %d (greater than %d)\n",
	      tid.id.task, MAX_TASK_ID);
      return -1;
    }
  task2app[tid.id.task] = i;

  printf("Task %x.%x (%08x:%08x) with diff. version number re-allocated\n",
         tid.id.task, tid.id.lthread, tid.lh.high, tid.lh.low);

  error = l4_ts_create(ts_id, (l4_ts_taskid_t*)&tid, 
		        (l4_umword_t)app, (l4_umword_t)esp, 0,
                        (l4_ts_taskid_t*)&pager, -1, "", 0, &exc);
  if (error || exc._type != exc_l4_no_exception)
    {
      printf("Error %d creating new task\n", error);
      return -1;
    }

  printf("Task %x.%x re-created\n", tid.id.task, tid.id.lthread);

  return 0;
}

