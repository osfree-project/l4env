/* $Id$ */
/**
 * \file	simple_ts/server/src/main.c
 *
 * \date	08/29/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> 
 *
 * \brief	server functions */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/generic_ts/generic_ts-server.h>
#include <l4/util/getopt.h>
#include <l4/util/bitops.h>

/* other includes */
#include <stdio.h>
#include <stdlib.h>

#ifdef L4API_l4x0
#define TASK_CNT_MAX	256
#else
#define TASK_CNT_MAX	512
#endif
#define TASK_CNT_DFL	64
#define TASK_FIRST	10

#define TASK_VECTYPE unsigned
#define TASK_VECSIZE (TASK_CNT_MAX+sizeof(TASK_VECTYPE)-1)/sizeof(TASK_VECTYPE)

static TASK_VECTYPE task_used[TASK_VECSIZE] = {0, };
static const unsigned taskno_min = TASK_FIRST;
static l4_threadid_t myself;
static l4_threadid_t task_owner[TASK_CNT_MAX];
static l4_threadid_t task_id[TASK_CNT_MAX];

static unsigned task_cnt = TASK_CNT_DFL;

/** parse command line arguments */
static void
parse_args(int argc, char **argv)
{
  static struct option long_options[] =
    {
      {"tasks", 1, 0, 't'},
      {0, 0, 0, 0},
    };

  while (1)
    {
      int c = getopt_long(argc, argv, "t:", long_options, NULL);
      char *endp;

      if (c == -1)
	break;

      switch (c) {
	case 't':
	  task_cnt = strtol(optarg, &endp, 0);
	  if (*endp)
	    {
	      printf("Invalid argument \"%s\" for tasks!\n", optarg);
	      exit(-1);
	    }
	  if (task_cnt > TASK_CNT_MAX)
	    {
              printf("Too many tasks (%d > %d)!\n", task_cnt, TASK_CNT_MAX);
              task_cnt = TASK_CNT_MAX;
              printf("Configured for %d tasks.\n", task_cnt);
	    }
	  break;
      }
    }
}

/** allocate task no */
static unsigned int
task_alloc(l4_threadid_t *caller, unsigned int wish)
{
  int n;
  if (wish != 0xffffffff)
    {
      n = wish - taskno_min;
      if (!l4util_test_and_set_bit(n, task_used))
	{
	  task_owner[n] = *caller;
	  return n + taskno_min;
	}
      return -L4_ENOTASK;
    }
  else
    {
      /* return any free task number */
      do 
	{
	  if ((n = l4util_find_first_zero_bit(task_used, task_cnt)) >= task_cnt)
	    /* no free task found */
	    return -L4_ENOTASK;
	} while (l4util_test_and_set_bit(n, task_used));
      task_owner[n] = *caller;
      return n + taskno_min;
    }
}

/** deliver task owner of task number n */
static l4_threadid_t
task_get_owner(int n)
{
  n -= taskno_min;
  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return L4_NIL_ID;
  if (!l4util_test_bit(n, task_used))
    /* task not used */
    return L4_NIL_ID;

  return task_owner[n];
}

/** free task no */
static int
task_free(l4_threadid_t *caller, int n)
{
  n -= taskno_min;
  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return -L4_ENOTFOUND;
  if (!l4util_test_and_clear_bit(n, task_used))
    /* task not used */
    return -L4_ENOTFOUND;
  if (!l4_task_equal(task_owner[n], *caller))
    /* task not owned by owner */
    return -L4_ENOTOWNER;
  task_owner[n] = L4_NIL_ID;
  return 0;
}

/** free all tasks which are owned by caller */
static int
task_free_all(l4_threadid_t *caller)
{
  printf("not implemented yet\n");
  return 0;
}

/** set priority of application */
static int
task_set_prio(l4_threadid_t *tid, int prio)
{
  return rmgr_set_prio(*tid, prio);
}

/** init structures */
static void
task_init(void)
{
  int error;
  unsigned int i;
  l4_threadid_t tid;
  for (i=0; i<task_cnt; i++)
    {
      unsigned int taskno = i+taskno_min;

      task_owner[i] = L4_NIL_ID;
      /* Set initial state of task id. */
      task_id[i].id = (l4_threadid_struct_t){ 
					      version_low:0, 
					      lthread:0, 
					      task:i+taskno_min, 
					      version_high:0, 
					      site:0, 
					      chief:rmgr_id.id.task, 
					      nest:0 
					    };
      if ((error = rmgr_get_task(taskno)))
	/* no permission for task, mark as used */
	l4util_set_bit(i, task_used);
      else
	{
	  /* this is a valid task number; however, as we create tasks
	   * through RMGR, just return the L4 task right back to RMGR */
	  tid = myself;
	  tid.id.chief = tid.id.task;
	  tid.id.task = taskno;
	  tid.id.nest++;
	  l4_task_new(tid, rmgr_id.lh.low, 0, 0, L4_NIL_ID);
	}
    }
}

/** Alloc task number
 * \param request	Flick request structure
 * \retval taskid	Task ID of new task
 * \return		0 on success 
 * 			-L4_ENOTASK if no task is available */
l4_int32_t 
l4_ts_allocate_component(CORBA_Object _dice_corba_obj,
			 l4_taskid_t *taskid,
			 CORBA_Environment *_dice_corba_env)
{
  int ret;
  l4_threadid_t *tid;

  /* allocate new task number */
  if ((ret = task_alloc(_dice_corba_obj, 0xffffffff)) < 0)
    return ret;

  tid = task_id + ret - taskno_min;
  tid->id.version_low++;

  *taskid = *tid;
  return 0;
}

/** Free task number
 *
 * \param request	Flick request structure
 * \param taskid	Task ID to free
 * \return		0 on succes
 * 			-L4_ENOTFOUND if task doesn't exist
 * 			-L4_ENOTOWNER if caller isn't the owner
 */
l4_int32_t 
l4_ts_free_component(CORBA_Object _dice_corba_obj,
		     const l4_taskid_t *taskid,
		     CORBA_Environment *_dice_corba_env)
{
  l4_threadid_t tid = *taskid;

  return task_free(_dice_corba_obj, tid.id.task);
}


/** Create an L4 task
 * \param request	Flick request structure
 * \param entry		initial program eip
 * \param stack		initial stack pointer
 * \param mcp_or_chief	maximum controlled priority
 * \param pager		initial pager
 * \retval taskid	Task ID of new task
 * \return		0 on success 
 * 			-L4_ENOTASK if no task is available */
l4_int32_t 
l4_ts_create_component(CORBA_Object _dice_corba_obj,
		       l4_taskid_t *taskid,
		       l4_uint32_t entry,
		       l4_uint32_t stack,
		       l4_uint32_t mcp,
		       const l4_taskid_t *pager,
		       l4_int32_t prio,
		       const char* resname,
		       l4_uint32_t flags,
		       CORBA_Environment *_dice_corba_env)
{
  l4_threadid_t tid = *taskid;

  if (  /* check if task is owned by client */
        !(l4_task_equal(task_get_owner(tid.id.task), *_dice_corba_obj)) 
	/* check if same task number as returned by task_allocate */
      ||!(l4_thread_equal(tid, task_id[tid.id.task-taskno_min]))
      )
    {
      printf("ERROR: task #%x doesn't match: client: %x.%02x, owner: %x.%02x\n",
	  tid.id.task,
	  _dice_corba_obj->id.task,
	  _dice_corba_obj->id.lthread,
	  task_get_owner(tid.id.task).id.task,
	  task_get_owner(tid.id.task).id.lthread);
      return -L4_EINVAL;
    }

  /* create an active L4 task */
  tid = rmgr_task_new(tid, mcp, stack, entry, *(l4_threadid_t*)pager);
  if (l4_is_nil_id(tid) || l4_is_invalid_id(tid))
    return -L4_EINVAL;

  /* set rmgr module name to identify task and to set quota */
  if (resname && *resname)
    {
      if (rmgr_set_task_id(resname, tid))
	printf("\"%s\" not configured at RMGR and therefore has no quotas\n", 
	    resname);
    }

  task_set_prio(&tid, prio);

  *taskid = tid;
  return 0;
}

/** Terminate task
 * \param request	Flick request structure
 * \param taskid	Task ID to free
 * \return		0 on success
 *			-L4_ENOTOWNER if not owner of that task number
 *			-L4_ENOTFOUND if task number was not found */
l4_int32_t 
l4_ts_delete_component(CORBA_Object _dice_corba_obj,
		       const l4_taskid_t *taskid,
		       CORBA_Environment *_dice_corba_env)
{
  int task;
  l4_threadid_t tid = *taskid;

  task = tid.id.task;
  tid = *_dice_corba_obj;
  tid.id.chief = _dice_corba_obj->id.task;
  tid.id.task = task;

  /* delete task (make inactive), return L4 task right back to RMGR */
  rmgr_task_new(tid, rmgr_id.lh.low, 0, 0, L4_NIL_ID);

  return 0;
}

/** Free all tasks the caller owns
 * \param request	Flick request structure
 * \return		0 on success */
l4_int32_t 
l4_ts_delete_all_component(CORBA_Object _dice_corba_obj,
    CORBA_Environment *_dice_corba_env)
{
  return task_free_all(_dice_corba_obj);
}

/** transmit right to create a task to the caller */
l4_int32_t 
l4_ts_get_task_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t taskno,
    CORBA_Environment *_dice_corba_env)
{
  return task_alloc(_dice_corba_obj, taskno);
}

/** transmit right to create a task back to the server */
l4_int32_t 
l4_ts_free_task_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t taskno,
    CORBA_Environment *_dice_corba_env)
{
  return task_free(_dice_corba_obj, taskno);
}

/** Main function, server loop. */
int
main(int argc, char **argv)
{
  /* init log lib */
  LOG_init("smpl_ts");

  parse_args(argc, argv);

  if (!rmgr_init())
    {
      printf("error initializing rmgr\n");
      return -L4_ENOTFOUND;
    }

  myself = l4_myself();

  /* get task id's from rmgr */
  task_init();

  /* register at nameserver */
  if (!names_register("SIMPLE_TS"))
    {
      printf("failed to register simple_ts!\n");
      return -1;
    }

  /* server loop */
  l4_ts_server_loop(NULL);

  return 0;
}

