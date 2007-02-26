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
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/thread/thread.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/util/stack.h>
#include <l4/util/l4_macros.h>

/* other includes */
#include <stdio.h>
#include <stdlib.h>

char LOG_tag[9] = "ts_test";

typedef struct 
{
  l4dm_dataspace_t	ds;
  l4_addr_t		start;
  l4_addr_t		end;
} stack_t;

#define MAX_TASK_CNT	1024
#define MAX_TASK_ID	1024

static stack_t app_stack[MAX_TASK_CNT];
static int task2app[MAX_TASK_ID] = {0, };

static void
app(int number, l4_threadid_t caller)
{
  l4_umword_t dw1, dw2;
  l4_msgdope_t result;
  l4_threadid_t src_id;
  l4_umword_t dummy;
  char s[64];

  sprintf(s, "  --> App %d: Hello World!\n", number);
  outstring(s);
  l4_ipc_call(caller, 
      	      L4_IPC_SHORT_MSG, 0, 0, 
	      L4_IPC_SHORT_MSG, &dummy, &dummy,
	      L4_IPC_NEVER, &result);
  for (;;)
    {
      /* receive anything from anywhere but don't answer */
      l4_ipc_wait(&src_id, L4_IPC_SHORT_MSG, &dw1, &dw2,
	  	  L4_IPC_NEVER, &result);
    }
}


static void
app_pager(void *unused)
{
  extern char _end;
  l4_umword_t dw1, dw2;
  void *reply_type;
  l4_msgdope_t result;
  l4_threadid_t src_thread;

  /* because of l4thread_create(..., L4THREAD_CREATE_SYNC) */
  l4thread_started(0);

  for (;;)
    {
      int error = l4_ipc_wait(&src_thread,
	                      L4_IPC_SHORT_MSG, &dw1, &dw2,
			      L4_IPC_NEVER, &result);
      while (!error)
	{
	  int appidx;

	  if (l4util_idtskstr(src_thread) > MAX_TASK_ID)
	    {
	      printf("pagefault at %08lx (eip %08lx) from unknown task "
		     l4util_idfmt"\n", dw1, dw2, l4util_idstr(src_thread));
	      enter_kdebug("stop");
	      continue;
	    }

	  /* implement Sigma0 pagefault protocol */
	  appidx = task2app[l4util_idtskstr(src_thread)];
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
	      printf("null pointer exception thread "l4util_idfmt
		     ", (%08lx at %08lx)\n", l4util_idstr(src_thread), dw1, dw2);
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
	      printf("unknown pagefault at %08lx (eip %08lx) from "l4util_idfmt
		     "\neip = %08lx..%08lx  stack = %08lx..%08lx\n",
		      dw1, dw2, l4util_idstr(src_thread),
		      (unsigned long)&app, (unsigned long)&task2app,
		      app_stack[appidx].start, app_stack[appidx].end);
	      enter_kdebug("stop");
	    }

	  error = l4_ipc_reply_and_wait(src_thread, reply_type, dw1, dw2,
	      				&src_thread, L4_IPC_SHORT_MSG,
					&dw1, &dw2, 
					L4_IPC_SEND_TIMEOUT_0,
					&result);
	}

      printf("pager: IPC error %x\n", error);
    }
}

int
main(void)
{
  int error, i;
  l4_threadid_t tid, pager, kill_tid;
  l4_addr_t esp;
  l4_umword_t dummy;
  l4_msgdope_t result;

  l4thread_set_prio(l4thread_myself(), 20);

  /* start pager thread */
  pager = l4thread_l4_id(l4thread_create(app_pager, 0, L4THREAD_CREATE_SYNC));

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
      esp = app_stack[i].start+L4_PAGESIZE;

      /* put app number and myself on top of stack as parameter */
      l4util_stack_push_threadid(&esp, l4_myself());
      l4util_stack_push_mword   (&esp, i);
      l4util_stack_push_mword   (&esp, 0);

      /* Allocate app number i. We can't allocate and create the task in
       * one step because after creating the task runs immediatly so it
       * may be that the new task's pager doesn't know anything about the
       * new task (because the task already runs before the task create to
       * the task server returns */
      if ((error = l4ts_allocate_task(&tid)))
	{
	  /* most probably we have reached the maximum number of tasks */
	  printf("Expected error allocating task: %d\n", error);
	  break;
	}

      if (tid.id.task > MAX_TASK_ID)
	{
	  printf("task number is "l4util_idtskfmt" (greater than "
		l4util_idtskfmt")\n", l4util_idtskstr(tid), MAX_TASK_ID);
	  return -1;
	}
      task2app[tid.id.task] = i;
      kill_tid = tid;

      printf("kill\n");
      /* Kill task. */
      if ((error = l4ts_kill_task(tid, L4TS_KILL_SYNC)))
	{
	  printf("Error %d killing task\n", error);
	  break;
	}
      printf("create\n");
      /* Create task. We choose a priority of 19 and an mcp of 0xe0. */
      if ((error = l4ts_create_task(&tid, (l4_addr_t)app, (l4_addr_t)esp, 
				    0xe0, &pager, 21, "",  0)))
	{
	  printf("Error %d creating task\n", error);
	  break;
	}
      
      /* shake hands with new app */
      l4_ipc_receive(tid, 0, &dummy, &dummy, L4_IPC_NEVER, &result);
      l4_ipc_send(tid, 0, 0, 0, L4_IPC_NEVER, &result);

      printf("Task "l4util_idfmt" stack at %08x..%08x is up\n",
	     l4util_idstr(tid),
	     (unsigned)esp & L4_PAGEMASK, 
	     ((unsigned)esp & L4_PAGEMASK)+L4_PAGESIZE-1);
    }

  /* give summary */
  printf("%d tasks created\n", i);

  tid = kill_tid;

  if ((error = l4ts_kill_task(tid, L4TS_KILL_SYNC)))
    {
      printf("error killing task "l4util_idfmt"\n", l4util_idstr(tid));
      return -1;
    }

  printf("Task "l4util_idfmt" killed\n", l4util_idstr(tid));

  if ((error = l4ts_free_task(&tid)))
    {
      printf("Error %d freeing task "l4util_idfmt"\n", 
	  error, l4util_idstr(tid));
      return -1;
    }
  
  printf("Task "l4util_idfmt" freed\n", l4util_idstr(tid));

  if ((error = l4ts_allocate_task(&tid)))
    {
      printf("Error %d allocating task "l4util_idfmt"\n", 
	      error, l4util_idstr(tid));
      return -1;
    }

  if (tid.id.task > MAX_TASK_ID)
    {
      printf("task number is "l4util_idtskfmt" (greater than %d)\n",
	      l4util_idtskstr(tid), MAX_TASK_ID);
      return -1;
    }
  task2app[tid.id.task] = i;

  printf("Task "l4util_idfmt" with diff. version number re-allocated\n",
         l4util_idstr(tid));

  if ((error = l4ts_create_task(&tid, (l4_addr_t)app, (l4_addr_t)esp, 
				0, &pager, -1, "", 0)))
    {
      printf("Error %d creating new task\n", error);
      return -1;
    }

  printf("Task "l4util_idfmt" re-created\n", l4util_idstr(tid));

  return 0;
}

