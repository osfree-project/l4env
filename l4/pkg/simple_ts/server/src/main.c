/* $Id$ */
/**
 * \file	simple_ts/server/src/main.c
 *
 * \date	08/29/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *		Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * \brief	Server functions
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/generic_ts/generic_ts-server.h>
#include <l4/generic_ts/generic_ts_server.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/util/bitops.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/util/thread.h>
#include <l4/util/parse_cmd.h>
#include <l4/events/events.h>

/* other includes */
#include <stdio.h>
#include <stdlib.h>

/* local includes */
#include "timeout_queue.h"

#ifdef L4API_l4x0
#define TASK_CNT_MAX	451
#else
#define TASK_CNT_MAX	901
#endif
#define TASK_CNT_DFL	64
#define TASK_FIRST	10

#define TASK_VECTYPE l4_umword_t
#define TASK_VECSIZE (TASK_CNT_MAX+sizeof(TASK_VECTYPE)-1)/sizeof(TASK_VECTYPE)

#define ANY_TASKNO	0xffffffff

char LOG_tag[9] = "simplets";

static TASK_VECTYPE   task_used[TASK_VECSIZE] = {0, };
static TASK_VECTYPE   task_term[TASK_VECSIZE] = {0, }; // task is terminating
static const unsigned taskno_min = TASK_FIRST;
static unsigned       task_cnt = TASK_CNT_DFL;
static char           task_name[TASK_CNT_MAX][16];

typedef struct __task
{
  l4_threadid_t		owner;
  l4_threadid_t		id;
  timeout_item_t	timeout;
} __task_t;

static __task_t __tasks[TASK_CNT_MAX];

static int no_owner_check;	/**< a task may be killed by any other task */
static int using_events;	/**< send exit event on termination of task */
static int verbose;		/**< be more verbose */

static l4_threadid_t server_id;
static l4_threadid_t events_id;
static l4_threadid_t ack_id;
static l4_threadid_t master_id = L4_INVALID_ID;

static char stack[L4_PAGESIZE] __attribute__((aligned(4)));

inline static unsigned
taskid_to_index(l4_threadid_t t)
{
  return (t.id.task - taskno_min);
}

inline static l4_threadid_t
index_to_taskid(unsigned n)
{
  return __tasks[n - taskno_min].id;
}

/** Parse command line arguments. */
static void
parse_args(int argc, const char **argv)
{
  int error;
  const char *master_name;

  if ((error = parse_cmdline(&argc, &argv,
		'a', "anykill", "no owner check on task kill",
		PARSE_CMD_SWITCH, 1, &no_owner_check,
		'e', "events", "use event server",
		PARSE_CMD_SWITCH, 1, &using_events,
		'm', "master", "set server which may kill all tasks",
		PARSE_CMD_STRING, "", &master_name,
		't', "tasks", "number of tasks allocated from RMGR",
		PARSE_CMD_INT, TASK_CNT_DFL, &task_cnt,
		'v', "verbose", "be more verbose",
		PARSE_CMD_SWITCH, 1, &verbose,
		0)))
    {
      switch (error)
	{
	case -1: LOG("Bad parameter for parse_cmdline()"); break;
	case -2: LOG("Out of memory in parse_cmdline()"); break;
	case -3:
	case -4: break;
	default: LOG("Error %d in parse_cmdline()", error); break;
	}
      exit(-1);
    }

  if (task_cnt + taskno_min > TASK_CNT_MAX)
    {
      LOG("Too many tasks (%d) -- limiting to %d!",
	  task_cnt, TASK_CNT_MAX - taskno_min);
      task_cnt = TASK_CNT_MAX - taskno_min;
    }

  printf("Configured for %d tasks.\n", task_cnt);

  if (master_name && *master_name)
    {
      if (!names_waitfor_name(master_name, &master_id, 10000))
	printf("Master server \"%s\" not found!\n", master_name);
    }
}

/** Send termination IPC to acknowledge thread. */
static void
send_to_ack_thread(l4_threadid_t client, int n, l4_uint8_t options)
{
  l4_msgdope_t result;
  timeout_item_t *i;

  /* setup first part of timeout item */
  i = &__tasks[n].timeout;
  i->client  = client;
  i->options = options;

  do
    {
      l4_ipc_send(ack_id, L4_IPC_SHORT_MSG, n, 0, L4_IPC_NEVER, &result);
    }
  while(L4_IPC_ERROR(result));
}

/** Init structures. */
static void
task_init(void)
{
  int error;
  unsigned int i;
  l4_threadid_t tid;
  for (i=0; i<task_cnt; i++)
    {
      l4_uint32_t taskno = i+taskno_min;

      __tasks[i].owner = L4_NIL_ID;
      /* Set initial state of task id. */
      __tasks[i].id.id = (l4_threadid_struct_t){
					      lthread:0,
					      task:i+taskno_min,
					      version_low:0,
					      version_high:0,
					    };
      __tasks[i].timeout.timeout = -1;

      if ((error = rmgr_get_task(taskno)))
	{
	  /* no permission for task, mark as used */
	  l4util_set_bit(i, task_used);
	  /* This is tricky: mark as terminating to be sure that we cannot
	   * kill that task. But even if we would try to kill that task
	   * the rmgr_task_new(..., L4_NIL_ID) call would fail. */
	  l4util_set_bit(i, task_term);
	}
      else
	{
	  /* this is a valid task number; however, as we create tasks
	   * through RMGR, just return the L4 task right back to RMGR */
	  tid = server_id;
	  tid.id.task = taskno;
	  l4_task_new(tid, (l4_umword_t)rmgr_id.raw, 0, 0, L4_NIL_ID);
	}
    }
}

/** Allocate taskno. */
static l4_uint32_t
task_alloc(l4_threadid_t *caller, l4_uint32_t taskno)
{
  unsigned n = taskno - taskno_min;

  if (taskno != ANY_TASKNO)
    {
      if (!l4util_test_and_set_bit(n, task_used))
	{
	  __tasks[n].owner = *caller;
	  __tasks[n].id.id.task = taskno;
	  __tasks[n].id.id.version_low++;
	  return taskno;
	}
      return -L4_ENOTASK;
    }

  /* return any free task number */
  do
    {
      if ((n = l4util_find_first_zero_bit(task_used, task_cnt)) >= task_cnt)
	/* no free task found */
	return -L4_ENOTASK;

    } while (l4util_test_and_set_bit(n, task_used));

  __tasks[n].owner = *caller;
  __tasks[n].id.id.version_low++;
  return n + taskno_min;
}

/** Free taskno. */
static int
task_free(l4_threadid_t *caller, l4_uint32_t taskno)
{
  unsigned n = taskno - taskno_min;

  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return -L4_ENOTFOUND;
  if (!l4_task_equal(__tasks[n].owner, *caller))
    /* task not owned by owner */
    return -L4_ENOTOWNER;
  if (!l4util_test_and_clear_bit(n, task_used))
    /* task not used */
    return -L4_ENOTFOUND;
  __tasks[n].owner = L4_NIL_ID;
  return 0;
}

/** Get task id of taskno. */
static l4_threadid_t
task_get_taskid(l4_uint32_t taskno)
{
  unsigned n = taskno - taskno_min;

  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return L4_NIL_ID;
  if (!l4util_test_bit(n, task_used))
    /* task not used */
    return L4_NIL_ID;

  return __tasks[n].id;
}

/** Get task owner of taskno. */
static l4_threadid_t
task_get_owner(l4_taskid_t taskid)
{
  unsigned n = taskid_to_index(taskid);

  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return L4_NIL_ID;
  if (!l4util_test_bit(n, task_used))
    /* task not used */
    return L4_NIL_ID;

  return __tasks[n].owner;
}

/** Get task owner of taskno. */
static int
task_set_owner(l4_taskid_t taskid, l4_taskid_t owner)
{
  unsigned n = taskid_to_index(taskid);

  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return -L4_EINVAL;
  if (!l4util_test_bit(n, task_used))
    /* task not used */
    return -L4_EINVAL;

  __tasks[n].owner = owner;
  return 0;
}

/** Stop and task free free used resources. */
static int
task_kill(l4_threadid_t caller, l4_taskid_t taskid, l4_uint8_t options)
{
  int n = taskid_to_index(taskid);

  if (n<0 || n>=task_cnt)
    /* invalid task no */
    return -L4_ENOTFOUND;

  if (!l4util_test_bit(n, task_used))
    return -L4_EUNKNOWN;

  /* test if task already terminating */
  if (l4util_test_bit(n, task_term))
    return -L4_EUNKNOWN;

  if (!no_owner_check &&
      !l4_task_equal(__tasks[n].owner, caller) &&
      !l4_task_equal(master_id, caller) &&
      !l4_task_equal(taskid, caller) &&
      !l4_is_nil_id(__tasks[n].owner))
    {
      LOG("Kill "l4util_idfmt" from "l4util_idfmt" but owner is "l4util_idfmt,
	  l4util_idstr(taskid), l4util_idstr(caller),
	  l4util_idstr(__tasks[n].owner));
      return -L4_ENOTOWNER;
    }

  /* set task to termination state */
  if (using_events && !(options & L4TS_KILL_NOEV))
    l4util_set_bit(n, task_term);

  /* Delete task (make inactive), return L4 task right back to RMGR.
   * Do this here to make sure that the task doesn't generate pagefaults
   * or other messages anymore. */
  if (rmgr_get_task(taskid.id.task))
    {
      LOG("Error deleting "l4util_idfmt" at RMGR", l4util_idstr(taskid));
      return -L4_EUNKNOWN;
    }
  /* as we create tasks through RMGR, return the L4 task right back to RMGR */
  l4_task_new(taskid, (l4_umword_t)rmgr_id.raw, 0, 0, L4_NIL_ID);

  /* Tell the ack thread that he should send the EXIT event to the event
   * server. We cannot make this ourself since the event server should
   * send an acknowledge after all resource managers has deallocated the
   * resources of the task to kill. */
  if (using_events && !(options & L4TS_KILL_NOEV))
    send_to_ack_thread(caller, n, options);
  else
    {
      /* If the ack_thread is not called we have to do that things ourself */
      if (options & L4TS_KILL_FREE)
	{
	  __tasks[n].owner = L4_NIL_ID;
	  l4util_clear_bit(n, task_used);
	}
    }

  return 0;
}

/** Exit task and all owned tasks. */
static int
task_kill_recursive(l4_threadid_t caller, l4_taskid_t taskid)
{
  int i, n = taskid_to_index(taskid);
  int ret;

  // per default free task resource
  if ((ret = task_kill(caller, taskid, L4TS_KILL_FREE)))
    return ret;

  /* kill owned tasks recursive */
  for (i=0; i<task_cnt; i++)
    if (l4util_test_bit(i, task_used) &&
	l4_task_equal(__tasks[i].owner, __tasks[n].id))
      task_kill_recursive(__tasks[n].id, __tasks[i].id);

  return 0;
}

/**
 *  Alloc task number.
 *
 * \retval taskid	Task ID of new task
 * \return		0 on success
 *			-L4_ENOTASK if no task is available
 */
long
l4_ts_allocate_component (CORBA_Object client,
                          l4_taskid_t *taskid,
                          CORBA_Server_Environment *_dice_corba_env)
{
  l4_int32_t taskno; /* UGLY, taskno can also be an error code */

  /* allocate new task number */
  if ((taskno = task_alloc(client, ANY_TASKNO)) < 0)
    return -L4_ENOTASK;

  *taskid = __tasks[taskno - taskno_min].id;
  return 0;
}

/** 
 * \brief Allocate a task ID and become the task's chief.
 * \retval taskid   allocated task ID
 * \return          0 on success
 *                  -L4_ENOTASK if no task is available
 */
long
l4_ts_allocate2_component(CORBA_Object client,
                          l4_taskid_t *taskid,
                          CORBA_Server_Environment *_dice_corba_env)
{
  l4_int32_t taskno;
  l4_taskid_t ret;

  if ((taskno = task_alloc(client, ANY_TASKNO)) < 0)
    return -L4_ENOTASK;

  /* Hmmm. During init() we returned all tasks to RMGR. This client
   * however wants to be the task's chief. Therefore we re-create
   * the task here. */
  if (rmgr_get_task(taskno))
    return -L4_ENOTASK;

  *taskid          = __tasks[taskno - taskno_min].id;
  // transfer chief rights
  ret = l4_task_new(*taskid, (l4_umword_t)client->raw, 0, 0, L4_NIL_ID);
  if (l4_is_nil_id(*taskid))
    {
      LOG_Error("Error allocating task with chief rights.");
      return -L4_ENOTASK;
    }

  __tasks[taskno - taskno_min].id = *taskid;
  return 0;
}

/**
 *  Free task number.
 *
 * \param taskid	Task ID to free
 * \return		0 on succes
 *			-L4_ENOTFOUND if task doesn't exist
 *			-L4_ENOTOWNER if caller isn't the owner
 */
long
l4_ts_free_component(CORBA_Object client, const l4_taskid_t *taskid,
		     CORBA_Server_Environment *_dice_corba_env)
{
  return task_free(client, taskid->id.task);
}

/**
 *  Get taskid for taskno.
 *
 * \param taskno	Task number
 * \retval taskid	Corresponding task ID
 */
long
l4_ts_taskno_to_taskid_component (CORBA_Object _dice_corba_obj,
                                  unsigned long tasknr,
                                  l4_taskid_t *taskid,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  *taskid = task_get_taskid(tasknr);
  return l4_is_invalid_id(*taskid) ? -L4_ENOTFOUND : 0;
}

/**
 *  Create an L4 task.
 *
 * \param request	Flick request structure
 * \param entry		initial program eip
 * \param stack		initial stack pointer
 * \param mcp_or_chief	maximum controlled priority
 * \param pager		initial pager
 * \retval taskid	Task ID of new task
 * \return		0 on success
 *			-L4_ENOTASK if no task is available
 */
long
l4_ts_create_component (CORBA_Object client,
                        l4_taskid_t *taskid,
                        l4_addr_t entry,
                        l4_addr_t stack,
                        unsigned long mcp,
                        const l4_taskid_t *pager,
                        const l4_taskid_t *caphandler,
                        long prio,
                        const char* resname,
                        unsigned long flags,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t tid = *taskid;

  if (  /* check if task is owned by client */
        !l4_task_equal(task_get_owner(tid), *client)
	/* check if same task number as returned by task_allocate */
      ||!l4_thread_equal(tid, __tasks[taskid_to_index(tid)].id))
    {
      printf("ERROR: task "l4util_idtskfmt" doesn't match: client "
	     l4util_idfmt", owner "l4util_idfmt"\n",
	     l4util_idtskstr(tid), l4util_idstr(*client),
	     l4util_idstr(task_get_owner(tid)));
      return -L4_EINVAL;
    }

  if (l4util_test_bit(taskid_to_index(tid), task_term))
    {
      printf("ERROR: task "l4util_idtskfmt" still terminating\n",
	     l4util_idtskstr(tid));
      return -L4_EUSED;
    }

  /* create an active L4 task */
  tid = rmgr_task_new_with_cap(tid, mcp, stack, entry, *pager, *caphandler);
  if (l4_is_nil_id(tid) || l4_is_invalid_id(tid))
    return -L4_EINVAL;

  /* set rmgr module name to identify task and to set quota */
  if (resname && *resname)
    {
      const char *f;

      if (rmgr_set_task_id(resname, tid))
	if (verbose)
	  printf("\"%s\" not configured at RMGR and therefore has no quotas\n",
		  resname);

      if (!(f = strrchr(resname, '/')))
	f = resname;
      else
	f++;
      snprintf(task_name[taskid_to_index(tid)], sizeof(task_name[0]), "%s", f);
    }

  rmgr_set_prio(tid, prio);

  *taskid = tid;
  return 0;
}

/**
 *  Terminate task.
 *
 * \param request	Flick request structure
 * \param taskid	Task ID to free
 * \return		0 on success
 *			-L4_ENOTOWNER if not owner of that task number
 *			-L4_ENOTFOUND if task number was not found
 */
long
l4_ts_kill_component (CORBA_Object client,
                      const l4_taskid_t *taskid,
                      l4_uint8_t options,
                      short *_dice_reply,
                      CORBA_Server_Environment *_dice_corba_env)
{
  int ret;

  ret = task_kill(*client, *taskid, options);
  if (using_events && !(options & L4TS_KILL_NOEV) &&
      ret == 0 && (options & L4TS_KILL_SYNC))
    *_dice_reply = DICE_NO_REPLY;
  else
    *_dice_reply = DICE_REPLY;
  return ret;
}

/**
 *  Terminate task for itself.
 *
 * \return		0 on success
 *			-L4_ENOTOWNER if not owner of that task number
 *			-L4_ENOTFOUND if task number was not found
 */
long
l4_ts_exit_component (CORBA_Object client,
                      short *_dice_reply,
                      CORBA_Server_Environment *_dice_corba_env)
{
  if (l4util_test_bit(taskid_to_index(*client), task_term))
    {
      LOG("Cannot exit "l4util_idfmt" (reserved or terminating)",
	  l4util_idstr(*client));
      *_dice_reply = DICE_NO_REPLY;
      return -L4_ENOTFOUND;
    }

  LOG_printf("Exit "l4util_idfmt"\n", l4util_idstr(*client));

  *_dice_reply = DICE_NO_REPLY;
  return task_kill_recursive(*client, *client);
}

/**
 *  Terminate task and terminate all owned tasks. There is no option to
 *  wait completely until all tasks are killed. The reason is that we
 *  would have to wait until _all_ child tasks and the task itself are
 *  killed.
 *
 * \param request	Flick request structure
 * \param taskid	Task ID to free
 * \return		0 on success
 *			-L4_ENOTOWNER if not owner of that task number
 *			-L4_ENOTFOUND if task number was not found
 */
long
l4_ts_kill_recursive_component (CORBA_Object client,
                                const l4_taskid_t *taskid,
                                CORBA_Server_Environment *_dice_corba_env)
{
  LOG_printf("Kill "l4util_idfmt" sent by "l4util_idfmt" owner "
      l4util_idfmt"\n",
      l4util_idstr(*taskid), l4util_idstr(*client),
      l4util_idstr(task_get_owner(*taskid)));
  return task_kill_recursive(*client, *taskid);
}

/**
 * Set owner of task.
 *
 * \param taskid	Task ID to change ownership of
 * \param taskid	Task ID of new owner
 * \return		0 on success
 *			-L4_ENOTOWNER if not owner of that task number
 */
long
l4_ts_owner_component (CORBA_Object client,
                       const l4_taskid_t *taskid,
                       const l4_taskid_t *owner,
                       CORBA_Server_Environment *_dice_corba_env)
{
  if (!l4_task_equal(task_get_owner(*taskid), *client))
    return -L4_ENOTOWNER;

  return task_set_owner(*taskid, *owner);
}


/**
 * Dump list of allocated tasks.
 */
void
l4_ts_dump_component(CORBA_Object client,
		     CORBA_Server_Environment *_dice_corba_env)
{
  unsigned int i;

  printf("Dumping all allocated tasks:\n");
  for (i=0; i<task_cnt; i++)
    {
      if (!l4_is_nil_id(__tasks[i].owner))
	{
	  int len = printf("  task-nr:%02x task:"l4util_idfmt
			   " owner:"l4util_idfmt,
			   i, l4util_idstr(__tasks[i].id),
			   l4util_idstr(__tasks[i].owner));
	  printf("%*s%s\n", 38-len, "",
	      *task_name[i] ? task_name[i] : "<unknown");
	}
    }
}

/**
 * Send one-way IPC back to the client which killed a task. This is only
 * possible in the context of the server (therefore the extra component
 * function).
 *
 * \param client	Task ID of client to answer
 * \return		0 on success
 */
void
l4_ts_do_kill_reply_component(CORBA_Object client, const l4_threadid_t *task,
                              CORBA_Server_Environment *_dice_corba_env)
{
  l4_ts_kill_reply ((l4_threadid_t*)task, 0, _dice_corba_env);
}


/**
 * This function searchs for the terminating task of an eventnr and
 * completes purging of the task resources. It clears the termination
 * bit, so the task server has the resource back for granting.
 */
static int
handle_ack(l4events_nr_t eventnr)
{
  timeout_item_t *i;

  for (i=timeout_first; i; i=i->next)
    {
      if (i->eventnr == eventnr)
	{
	  /* manually call the RMGR to free all resources of the task
	   * because the RMGR is not able to connect to the event server */
	  rmgr_free_irq_all(__tasks[i->index].id);
	  rmgr_free_mem_all(__tasks[i->index].id);
	  rmgr_free_task_all(__tasks[i->index].id);

	  unlink_item_from_timeout_queue(i);

	  /* make task resource available task server */
	  l4util_clear_bit(i->index, task_term);

	  /* task number is free again */
	  if (i->options & L4TS_KILL_FREE)
	    {
	      __tasks[i->index].owner = L4_NIL_ID;
	      l4util_clear_bit(i->index, task_used);
	    }

	  /* ask the server to notify the client that the kill
	   * call was completed */
	  if (i->options & L4TS_KILL_SYNC)
	    l4ts_do_kill_reply(&server_id, &i->client);

	  return 1;
	}
    }

  return 0;
}

/**
 * This function checks if timeout for an exit event is running down.
 * Actually this function is not doing any useful thing. It only sets up
 * a new fresh timeout for the exit event.
 */
static void
handle_timeout(void)
{
  timeout_item_t *i;

  timeout_first->timeout--;

  /* sanity check */
  if (timeout_first->timeout < 0)
    {
      LOG("some problems in timeout queue");
      enter_kdebug("SIMPLE_TS PANIC");
    }

  while (timeout_first->timeout == 0)
    {
      i = timeout_first;

      printf("\033[41mExit NOT SUCCESSFUL done for "l4util_idtskfmt"!\033[m\n",
	  l4util_idtskstr(__tasks[i->index].id));

      /* we are doing nothing more than a relink of the timeout
       * at the moment */
      unlink_item_from_timeout_queue(i);
      link_item_to_timeout_queue(i);
    }
}

/**
 * Tell the event server that we want to be notified if task t was completely
 * terminated (i.e. all resources of all registered resource managers are
 * freed). Setup a timeout item for bookkeeping outstanding exit events.
 */
static int
send_exit_event(int n)
{
  l4events_event_t event;
  l4events_nr_t eventnr;
  timeout_item_t *i;
  int ret;

  /* sent exit_event */
  event.len = sizeof(l4_threadid_t);
  *(l4_threadid_t*)event.str = __tasks[n].id;

  if ((ret = l4events_send(L4EVENTS_EXIT_CHANNEL, &event, &eventnr,
			   L4EVENTS_SEND_ACK)))
    return -L4_EUNKNOWN;

  /* setup second part of timeout item */
  i = &__tasks[n].timeout;
  i->index   = n;
  i->eventnr = eventnr;
  i->timeout = -1;

  link_item_to_timeout_queue(i);
  return 0;
}

/**
 * The acknowledge thread. Used to communicate with the event server.
 * In normal case, we wait for a message (exit event) from the event
 * server --- like the event thread in the resource managers. But
 * furthermore we also may get a message from the server that we should
 * send an exit event to the event server.
 */
static void
ack_thread(void)
{
  l4_threadid_t sender;
  l4_umword_t w1, w2;
  l4events_nr_t eventnr;
  l4_msgdope_t result;

  for (;;)
    {
      if (!timeout_first)
	{
	  /* There is no pending task to be killed. Wait for a new request
	   * of the server. */
	  l4_ipc_receive(server_id, L4_IPC_SHORT_MSG, &w1, &w2,
			 L4_IPC_NEVER, &result);
	}
      else
	{
	  /* There are pending tasks to be killed. Wait until the event server
	   * notifies us that a task was killed. Ensure that we don't wait
	   * forever. */
	  eventnr = timeout_first->eventnr;
          if (l4events_get_ack_open(&eventnr, &sender, &w1, &w2,
				    l4_ipc_timeout(0,0,976,10) /* 1s */)
	      == -L4EVENTS_ERROR_TIMEOUT)
	    {
	      handle_timeout();
	      continue;
	    }

	  if (l4_thread_equal(sender, events_id))
	    {
	      /* The event server notified us that a task was terminated. */
	      handle_ack(eventnr);
	      continue;
	    }

	  if (!l4_thread_equal(sender, server_id))
	    {
	      LOG("Ignoring message from unexpected sender "l4util_idfmt,
		  l4util_idstr(sender));
	      continue;
	    }
	}

      /* Got new taskno from the server. Tell the event server that we want
       * to get notified when the task was completely terminated (all
       * registered servers for the exit event did successfully reply) */
      send_exit_event(w1);
    }
}

static void
get_events_id(void)
{
  if (!names_waitfor_name(L4EVENTS_SERVER_NAME, &events_id, 10000))
    {
      printf("No events server found\n");
      enter_kdebug("STOP");
    }
}

void
dice_server_error(l4_msgdope_t result, CORBA_Server_Environment* env)
{
  /* our client was killed, perhaps due exit handling */
  if (result.msgdope == L4_IPC_ENOT_EXISTENT)
    return;

  LOG("server error: %lx", result.msgdope);
}

/** Main function, server loop. */
int
main(int argc, const char **argv)
{
  parse_args(argc, argv);

  if (!rmgr_init())
    {
      printf("error initializing rmgr\n");
      return -L4_ENOTFOUND;
    }

  server_id = l4_myself();

  /* get task id's from rmgr */
  task_init();

  /* setting up timeout thread */
  if (using_events)
    {
      get_events_id();
      ack_id = l4util_create_thread(1, ack_thread, (int*)(stack+L4_PAGESIZE));
      names_register_thread_weak("simplets.events", ack_id);
    }

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
