/******************************************************************************
 * Bjoern Doebel <doebel@tudos.org>                                           *
 *                                                                            *
 * (c) 2005 - 2007 Technische Universitaet Dresden                            *
 * This file is part of DROPS, which is distributed under the terms of the    *
 * GNU General Public License 2. Please see the COPYING file for details.     *
 *                                                                            *
 * Task server client lib. Implements the generic_ts library functions        *
 ******************************************************************************/

#include <l4/sys/syscalls.h>
#include <l4/crtx/ctor.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/task/task_client.h>
#include "generic_ts-client.h"

#include "tsclient.h"

#define IPCCAP_HACK 1

/******************************************************************************
 * callback handler functions ´                                               *
 ******************************************************************************/
static l4task_client_callbacks callbacks;

static inline long allocate_callback(unsigned int taskno, l4_taskid_t *t, char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.client_alloc_callback)
		return callbacks.client_alloc_callback(taskno, t, status);
	return 0;
}


static inline long free_callback(const l4_taskid_t *t, char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.client_free_callback)
		return callbacks.client_free_callback(t, status);
	return 0;
}


static inline long exit_callback(char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.client_exit_callback)
		return callbacks.client_exit_callback(status);
	return 0;
}


static inline long kill_callback(l4_taskid_t *task, l4_uint8_t options,
                                 char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.client_kill_callback)
		return callbacks.client_kill_callback(task, options, status);
	return 0;
}


static inline long create_callback(l4_taskid_t *task, l4_addr_t eip,
                                   l4_addr_t esp, l4_uint32_t mcp,
                                   const l4_taskid_t *pager,
                                   const l4_taskid_t *caphandler,
                                   l4_int32_t prio, const char *name,
                                   l4_uint32_t flags, char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.client_create_callback)
		return callbacks.client_create_callback(task, eip, esp, mcp, pager,
		                                        caphandler, prio, name,
		                                        flags, status);
	return 0;
}

int l4task_client_register_hooks(l4task_client_callbacks cb)
{
	callbacks = cb;
	return 0;
}


/******************************************************************************
 * generic_ts functions                                                       *
 ******************************************************************************/

/** Allocate a task.
 *
 * This calls our parent and retrieves a valid L4 task id from it.
 * We are then this task's chief and are allowed to start/stop it
 * using l4_task_new().
 */
int l4ts_allocate_task(unsigned int taskno, l4_taskid_t *t)
{
	DICE_DECLARE_ENV(env);
	int err;
	char status;

	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	/* Try callback. */
	err = allocate_callback(taskno, t, &status);
	if (status == CALLBACK_SKIP)
		return err;

	err = l4_ts_allocate_call(&l4ts_server_id, taskno, t, &env);
	DEBUG_MSG("alloc called: %d, "l4util_idfmt, err, l4util_idstr(*t));

#if IPCCAP_HACK
	if (!l4_is_invalid_id(*t)) {
		l4_threadid_t d = *t;
		l4_msgdope_t m;
		d.id.lthread = 0;
		int r = l4_ipc_send(d, L4_IPC_SHORT_MSG, 23, 42, L4_IPC_BOTH_TIMEOUT_0, &m);
		DEBUG_MSG("fake cap send returned %d", r);
	}
#endif

	return err;
}


/** The same as \ref l4ts_allocate_task. */
int l4ts_allocate_task2(unsigned int taskno, l4_taskid_t *t)
{
	return l4ts_allocate_task(taskno, t);
}


int
l4ts_create_task(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
                 l4_uint32_t mcp, const l4_taskid_t *pager, l4_int32_t prio,
                 const char *resname, l4_uint32_t flags)
{
	int err;
	char status;
	l4_taskid_t n, s = L4_INVALID_ID;
	l4_sched_param_t sched;
	l4_taskid_t nil_cap = L4_NIL_ID;

	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	DEBUG_MSG("creating task "l4util_idfmt, l4util_idstr(*taskid));

	if (l4_is_nil_id(*pager)) {
		LOG("INVALID PAGER");
		return -L4_EBADF;
	}

	/* Try callback. */
	err = create_callback(taskid, entry, stack, mcp, pager, &nil_cap,
	                      prio, resname, flags, &status);
	if (status == CALLBACK_SKIP)
		return err;

	n = l4_task_new(*taskid, mcp|flags, stack, entry, *pager);

	if (l4_is_nil_id(n)) {
		LOG("Error creating task "l4util_idfmt, l4util_idstr(*taskid));
		return -L4_ENOMEM;
	}

	/* Setup scheduling information */
	l4_thread_schedule(*taskid, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
	sched.sp.prio = prio;
	s = L4_INVALID_ID;
	l4_thread_schedule(*taskid, sched, &s, &s, &sched);

	return 0;
}


int
l4ts_create_task2(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
                  l4_uint32_t mcp, const l4_taskid_t *pager,
                  const l4_taskid_t *caphandler, l4_quota_desc_t kquota,
                  l4_int32_t prio, const char *resname, l4_uint32_t flags)
{
	int err;
	char status;
	l4_taskid_t n, s = L4_INVALID_ID;
	l4_sched_param_t sched;

	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	DEBUG_MSG("creating task "l4util_idfmt, l4util_idstr(*taskid));
	if (l4_is_nil_id(*pager)) {
		LOG("INVALID PAGER");
		return -L4_EBADF;
	}

	/* Try callback. */
	err = create_callback(taskid, entry, stack, mcp, pager, caphandler,
	                      prio, resname, flags, &status);
	if (status == CALLBACK_SKIP)
		return err;

	n = l4_task_new_long(*taskid, mcp|flags, stack, entry, *pager,
	                     *caphandler, kquota, l4_utcb_get());

	if (l4_is_nil_id(n)) {
		LOG("Error creating task "l4util_idfmt, l4util_idstr(*taskid));
		return -L4_ENOMEM;
	}

	/* Setup scheduling information */
	l4_thread_schedule(*taskid, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
	sched.sp.prio = prio;
	s = L4_INVALID_ID;
	l4_thread_schedule(*taskid, sched, &s, &s, &sched);

	return 0;
}


/** Return task to parent. */
int l4ts_free_task(const l4_taskid_t *t)
{
	int err;
	char status;

	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	l4_threadid_t ret;
	DICE_DECLARE_ENV(env);

	/* Try callback. */
	err = free_callback(t, &status);
	if (status == CALLBACK_SKIP)
		return err;

	/* return chieftainship */
	DEBUG_MSG("returning ownership of "l4util_idfmt" to "l4util_idfmt,
	l4util_idstr(*t), l4util_idstr(l4ts_server_id));
	ret = l4_task_new(*t, (unsigned)l4ts_server_id.raw, 0, 0, L4_NIL_ID);
	if (l4_is_nil_id(ret)) {
		LOG("Could not return task.");
		return -1;
	}

	/* notify parent */
	err = l4_ts_free_call(&l4ts_server_id, &ret, &env);
	return err;
}


/** Exit task.
 *
 * Exitting means to notify our parent which will then kill us and
 * retain all our subtasks.
 */
int l4ts_exit(void)
{
	int err;
	char status;
	DICE_DECLARE_ENV(env);

	DEBUG_MSG("task "l4util_idfmt" calls exit()\n",
		 l4util_idstr(l4_myself()));

	err = exit_callback(&status);
	if (status == CALLBACK_SKIP)
		/* nothing in this case */;

	err = l4_ts_exit_call(&l4ts_server_id, &env);

	if (err || DICE_HAS_EXCEPTION(&env)) {
		LOG_Error("Cannot exit. server = "l4util_idfmt", ret = %d, exc %d",
		          l4util_idstr(l4ts_server_id), err, DICE_EXCEPTION_MAJOR(&env));
	}

	l4_sleep_forever();
}


int l4ts_owner(l4_taskid_t task, l4_taskid_t owner)
{
	/* Do nothing. Client is able to manage the new task using our local
	 * taskserver interface.
	 */
	return 0;
}


/** Kill a task that has been started by ourselves. */
int l4ts_kill_task(l4_taskid_t task, l4_uint8_t options)
{
	l4_threadid_t ret;
	int err;
	char status;

	DEBUG_MSG(l4util_idfmt" is killing task "l4util_idfmt".",
	          l4util_idstr(l4_myself()), l4util_idstr(task));

	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	err = kill_callback(&task, options, &status);
	if (status == CALLBACK_SKIP)
		return err;

	/* Delete task. */
	ret = l4_task_new(task, l4_myself().raw, 0, 0, L4_NIL_ID);
	if (l4_is_nil_id(ret)) {
		LOG_Error("Cannot kill task "l4util_idfmt, l4util_idstr(task));
		return -L4_EBADF;
	}

	return 0;
}


int l4ts_kill_task_recursive(l4_taskid_t task)
{
	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	return l4ts_kill_task(task, 0);
}


int l4ts_dump_tasks(void)
{
	if (!l4ts_connected())
		return -L4_ENOTFOUND;

	LOG_Enter();
	return -1;
}

