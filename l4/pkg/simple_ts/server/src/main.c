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

/* other includes */
#include <stdio.h>
#include <stdlib.h>

/* private includes */
#include "bitops.h"

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
      if (!test_and_set_bit(n, task_used))
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
	  if ((n = find_first_zero_bit(task_used, task_cnt)) >= task_cnt)
	    /* no free task found */
	    return -L4_ENOTASK;
	} while (test_and_set_bit(n, task_used));
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
  if (!test_bit(n, task_used))
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
  if (!test_and_clear_bit(n, task_used))
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
	set_bit(i, task_used);
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
l4_int32_t l4_ts_server_allocate(sm_request_t *request,
			      l4_ts_taskid_t *taskid, sm_exc_t *_ev)
{
  int ret;
  l4_threadid_t *tid;

  /* allocate new task number */
  if ((ret = task_alloc(&request->client_tid, 0xffffffff)) < 0)
    return ret;

  tid = task_id + ret - taskno_min;
  tid->id.version_low++;

  *taskid = *((l4_ts_taskid_t*)tid);
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
l4_int32_t l4_ts_server_free(sm_request_t *request,
			   const l4_ts_taskid_t *taskid, sm_exc_t *_ev)
{
  l4_threadid_t tid = *(l4_threadid_t *)taskid;

  return task_free(&request->client_tid, tid.id.task);
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
l4_int32_t l4_ts_server_create(sm_request_t *request, l4_ts_taskid_t *taskid,
			       l4_uint32_t entry, l4_uint32_t stack, 
			       l4_uint32_t mcp, const l4_ts_taskid_t *pager, 
			       l4_int32_t prio, const char *resname, 
			       l4_uint32_t flags, sm_exc_t *_ev)
{
  l4_threadid_t tid = *((l4_threadid_t*)taskid);

  if (  /* check if task is owned by client */
        !(l4_task_equal(task_get_owner(tid.id.task), request->client_tid)) 
	/* check if same task number as returned by task_allocate */
      ||!(l4_thread_equal(tid, task_id[tid.id.task-taskno_min]))
      )
    {
      printf("task: %x, client: %x.%x, owner: %x.%x\n",
	  tid.id.task,
	  request->client_tid.id.task,
	  request->client_tid.id.lthread,
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

  *taskid = *((l4_ts_taskid_t*)&tid);
  return 0;
}

/** Terminate task
 * \param request	Flick request structure
 * \param taskid	Task ID to free
 * \return		0 on success
 *			-L4_ENOTOWNER if not owner of that task number
 *			-L4_ENOTFOUND if task number was not found */
l4_int32_t l4_ts_server_delete(sm_request_t *request, 
			     const l4_ts_taskid_t *taskid, sm_exc_t *_ev)
{
  int task;
  l4_threadid_t tid = *(l4_threadid_t*)taskid;

  task = tid.id.task;
  tid = request->client_tid;
  tid.id.chief = request->client_tid.id.task;
  tid.id.task = task;

  /* delete task (make inactive), return L4 task right back to RMGR */
  rmgr_task_new(tid, rmgr_id.lh.low, 0, 0, L4_NIL_ID);

  return 0;
}

/** Free all tasks the caller owns
 * \param request	Flick request structure
 * \return		0 on success */
l4_int32_t l4_ts_server_delete_all(sm_request_t *request, sm_exc_t *_ev)
{
  return task_free_all(&request->client_tid);
}

/** transmit right to create a task to the caller */
l4_int32_t l4_ts_server_get_task(sm_request_t *request, 
			       l4_uint32_t taskno, sm_exc_t *_ev)
{
  return task_alloc(&request->client_tid, taskno);
}

/** transmit right to create a task back to the server */
l4_int32_t l4_ts_server_free_task(sm_request_t *request, 
				l4_uint32_t taskno, sm_exc_t *_ev)
{
  return task_free(&request->client_tid, taskno);
}

/** Main function, server loop. */
int
main(int argc, char **argv)
{
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;
  int ret;

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
  flick_init_request(&request, &ipc_buf);
  for (;;)
    {
      result = flick_server_wait(&request);

      while (!L4_IPC_IS_ERROR(result))
	{
#if DEBUG_REQUEST
	  printf("request 0x%08x, src %x.%x\n",ipc_buf.buffer[0],
	         request.client_tid.id.task,request.client_tid.id.lthread);
#endif

	  /* dispatch request */
	  ret = l4_ts_server(&request);
	  switch(ret)
	    {
	    case DISPATCH_ACK_SEND:
	      /* reply and wait for next request */
	      result = flick_server_reply_and_wait(&request);
	      break;
	      
	    default:
	      printf("Flick dispatch error (%d)!\n",ret);
	      
	      /* wait for next request */
	      result = flick_server_wait(&request);
	      break;
	    }
	}
      printf("Flick IPC error (0x%08x)!\n",result.msgdope);
    }

  return 0;
}

