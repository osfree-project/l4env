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
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/util/stack.h>
#include <l4/generic_ts/generic_ts-client.h>
#include <l4/dm_phys/dm_phys.h>

/* other includes */
#include <stdio.h>
#include <stdlib.h>

char LOG_tag[9] = "ts_exit";

static int debug_pager = 0;

typedef struct
{
  l4_addr_t		start;
  l4_addr_t		end;
} stack_t;

#define MAX_TASK_CNT	1024
#define MAX_TASK_ID	1024

static stack_t app_stack[MAX_TASK_CNT];
static l4_addr_t app_stack_mem;
static l4dm_dataspace_t app_stack_ds;
static int task2app[MAX_TASK_ID] = {0, };
static l4_threadid_t app2task[MAX_TASK_CNT] = {L4_INVALID_ID, };
static int appindex = 1;
static l4_threadid_t ts_id;
static l4_threadid_t dsm_id;
static l4_threadid_t pager;
#define DS_SIZE 1024

static void
app(int id, int num_create, l4_threadid_t caller)
{
  l4_msgdope_t result;
  l4_umword_t dummy;
  int i;
  char name[NAMES_MAX_NAME_LEN];
  l4dm_dataspace_t ds;
  l4_quota_desc_t invquota = L4_INVALID_KQUOTA;

  printf("--> App %d: Hello World!\n", id);

  sprintf(name, "simple_ts_exit_%i", id);
  names_register(name);
  l4dm_mem_open(dsm_id, DS_SIZE, 0, 0, name, &ds);

  for (i=0; i<num_create; i++)
    {
      l4_addr_t esp = app_stack[appindex].start+L4_PAGESIZE;
      CORBA_Environment _env = dice_default_environment;
      l4_threadid_t tid, invalid = L4_INVALID_ID;
      int error;

      /* put app number and myself on top of stack as parameter */
      l4util_stack_push_threadid(&esp, l4_myself());
      l4util_stack_push_mword   (&esp, num_create-1);
      l4util_stack_push_mword   (&esp, appindex);
      l4util_stack_push_mword   (&esp, 0);

      /* Allocate app number i. We can't allocate and create the task in
       * one step because after creating the task runs immediatly so it
       * may be that the new task's pager doesn't know anything about the
       * new task (because the task already runs before the task create to
       * the task server returns */
      if ((error = l4_ts_allocate_call(&ts_id, &tid, &_env))
	  || DICE_HAS_EXCEPTION(&_env))
	{
	  /* most probably we have reached the maximum number of tasks */
	  printf("Error allocating task: %d\n", error);
	  break;
	}

      if (tid.id.task > MAX_TASK_ID)
	{
	  printf("task number is %d (greater than %d)\n",
	        tid.id.task, MAX_TASK_ID);
	  exit(-1);
	}
      task2app[tid.id.task] = appindex;
      app2task[appindex] = tid;

      appindex++;

      /* Create task. We choose a priority of 19 and an mcp of 0xe0. */
      if ((error = l4_ts_create_call(&ts_id, &tid,
				     (l4_addr_t)app, esp, 0xe0,
				     &pager,
                                     &invalid, &invquota, 21, "",  0, &_env))
	  || DICE_HAS_EXCEPTION(&_env))
	{
	  printf("Error %d creating task\n", error);
	  break;
	}

      /* shake hands with new app */
      l4_ipc_receive(tid, 0, &dummy, &dummy, L4_IPC_NEVER, &result);

      if (debug_pager)
        printf("Task "l4util_idfmt" (%08x) stack at %08lx..%08lx is up\n",
	     l4util_idstr(tid), tid.raw,
	     esp & L4_PAGEMASK, (esp & L4_PAGEMASK)+L4_PAGESIZE-1);
    }

  if (id != 0)
    {
      l4_ipc_send(caller,
		  L4_IPC_SHORT_MSG, 0, 0,
		  L4_IPC_NEVER, &result);

      l4_sleep_forever();
    }
}


static void
app_pager(void *unused)
{
  extern char _end[];
  extern char _stext[];
  extern char _etext[];
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

	  if (src_thread.id.task > MAX_TASK_ID)
	    {
	      printf("pagefault at %08lx (eip %08lx) from unknown task "
		     l4util_idfmt"\n", dw1, dw2, l4util_idstr(src_thread));
	      enter_kdebug("stop");
	      continue;
	    }

	  /* implement Sigma0 pagefault protocol */
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
	      printf("null pointer exception thread "l4util_idfmt
		     ", (%08lx at %08lx)\n",
		     l4util_idstr(src_thread), dw1, dw2);
	      enter_kdebug("stop");
	    }
	  else if ((dw1 >= (l4_umword_t)&_stext) &&
		   (dw1  < (l4_umword_t)&_etext))
	    {
	      /* pf in text section (a bit tricky, normally we should
	       * have knowledge about the ELF file sections) */
	      dw1 &= L4_PAGEMASK;
	      dw2 = l4_fpage(dw1, L4_LOG2_PAGESIZE,
		                  L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
	    }
	  else if ((dw1 >= (l4_umword_t)&_etext) &&
	           (dw1 < (l4_umword_t)&_end))
	    {
	      /* pf in data section (remarks like text section) */
	      dw1 &= L4_PAGEMASK;
	      dw2 = l4_fpage(dw1, L4_LOG2_PAGESIZE,
		                  L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
	    }
	  else if ((dw1 >= app_stack_mem) &&
	           (dw1 <  app_stack_mem + MAX_TASK_CNT*L4_PAGESIZE))
	    {
	      dw1 &= L4_PAGEMASK;
	      dw2 = l4_fpage(dw1, L4_LOG2_PAGESIZE,
		  		  L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
	    }
	  else
	    {
	      printf("unknown pagefault at %08lx (eip %08lx) from "
		     l4util_idfmt"\n"
		     "eip = %08lx..%08lx  stack = %08lx..%08lx\n",
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
  l4_threadid_t my_id;
  int i;
  CORBA_Environment _env = dice_default_environment;

  if (!names_waitfor_name("SIMPLE_TS", &ts_id, 5000))
    {
      printf("SIMPLE_TS not found\n");
      return -1;
    }

  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    {
      printf("dm_phys not found!");
      return -1;
    }
  my_id = l4_myself();
  l4thread_set_prio(l4thread_myself(), 20);

  /* start pager thread */
  pager = l4thread_l4_id(l4thread_create(app_pager, 0, L4THREAD_CREATE_SYNC));

  printf("Pager is up.\n");

  app_stack_mem = (l4_addr_t)l4dm_mem_ds_allocate(MAX_TASK_CNT*L4_PAGESIZE,
						  0, &app_stack_ds);
  l4_touch_rw((const void*)app_stack_mem, MAX_TASK_CNT*L4_PAGESIZE);

  if (app_stack_mem == 0)
    {
      printf("error allocating memory for stack");
      exit(-1);
    }

  for (i=0;i<MAX_TASK_CNT;i++)
    {
      /* allocate one page as stack for new task */
      app_stack[i].start = app_stack_mem + i*L4_PAGESIZE;
      app_stack[i].end   = app_stack_mem + i*L4_PAGESIZE + L4_PAGESIZE - 1;
    }

  app(0, 3, L4_INVALID_ID);

  printf("\n");
  l4_ts_dump_call(&ts_id, &_env);
  printf("\n");
  names_dump();
  printf("\n");
  l4dm_ds_list_all(dsm_id);
  printf("\n exit task-nr:1 (and all sub-tasks)\n");
  l4_ts_kill_recursive_call(&ts_id, &app2task[1], &_env);
  printf("\n");
  l4_ts_dump_call(&ts_id, &_env);
  printf("\n");
  names_dump();
  printf("\n");
  l4dm_ds_list_all(dsm_id);
  printf("\n");

  printf("Well done!\n");

  return 0;
}

