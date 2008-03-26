/* vim:set ts=4: */
/******************************************************************************
 * Bjoern Doebel <doebel@tudos.org>                                           *
 *                                                                            *
 * (c) 2005 - 2007 Technische Universitaet Dresden                            *
 * This file is part of DROPS, which is distributed under the terms of the    *
 * GNU General Public License 2. Please see the COPYING file for details.     *
 *                                                                            *
 * Task server lib. Simply route all calls to our parent and keep track of    *
 * which tasks we handed out to our clients.                                  *
 ******************************************************************************/

#include <l4/crtx/ctor.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/task/task_server.h>
#include "generic_ts-server.h"
#include "generic_ts-client.h"

#define TASKSERVER_PARENT_TID   3
#define RMGR_DEFAULT_ID         4

#define MAX_SUBTASKS            96
#define MAX_CLIENTS             16

#define DEBUG                   0

typedef struct {
	l4_threadid_t   id;                       /**< client ID */
	l4_taskid_t     subtasks[MAX_SUBTASKS];   /**< subtasks allocated to client */
} client_t;

static l4task_server_callbacks callbacks = { .allocate_callback = NULL,
                                             .post_alloc_callback = NULL,
                                             .free_callback = NULL,
                                             .exit_callback = NULL };

static l4_threadid_t l4task_server_thread = L4_INVALID_ID;

/** Client-subtask mapping */
static client_t children[MAX_CLIENTS];

l4_threadid_t l4task_get_server(void)
{
	return l4task_server_thread;
}

/******************************************************************************
 * Callback management                                                        *
 ******************************************************************************/
static inline long allocate_callback(l4_threadid_t client, unsigned int taskno,
                                     l4_taskid_t *task, char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.allocate_callback)
		return callbacks.allocate_callback(client, taskno, task, status);
	return 0;
}


static inline void post_allocate_callback(l4_threadid_t client, l4_taskid_t *task)
{
	if (callbacks.post_alloc_callback)
		callbacks.post_alloc_callback(client, task);
}


static inline long free_callback(l4_threadid_t client, const l4_taskid_t *task, char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.free_callback)
		return callbacks.free_callback(client, task, status);
	return 0;
}


static inline long exit_callback(l4_threadid_t client, char *status)
{
	*status = CALLBACK_CONTINUE;
	if (callbacks.exit_callback)
		return callbacks.exit_callback(client, status);
	return 0;
}


int l4task_server_register_hooks(l4task_server_callbacks cb)
{
	callbacks = cb;
	return 0;
}

/******************************************************************************
 * Internal client handling.                                                  *
 ******************************************************************************/

/** Find client in list.
 *
 * Find the client task in list of clients. Return NULL if not found.
 */
static client_t *find_client(l4_taskid_t clnt)
{
	int i;

	for (i=0; i<MAX_CLIENTS; i++) {
		if (l4_task_equal(children[i].id, clnt))
			return &children[i];
	}

	return NULL;
}


/** Add client.
 *
 * Add client task to list of known clients. Return pointer to client
 * struct or NULL if out of memory.
 */
static client_t *add_client(l4_taskid_t clnt)
{
	int i;

	for (i=0; i<MAX_CLIENTS; i++) {
		if (l4_task_equal(children[i].id, L4_INVALID_ID)) {
			int j;
			children[i].id = clnt;
			for (j=0; j<MAX_SUBTASKS; j++)
				children[i].subtasks[j] = L4_INVALID_ID;
			return &children[i];
		}
	}

	return NULL;
}


/** Add task to client's subtasks.
 *
 * Add task to be a subtask of client so that we can remove it if client
 * terminates or gets killed.
 *
 * \return 0  success
 *         -1 no such task
 */
static int add_task_for_client(client_t *clnt, l4_taskid_t task)
{
	int i;

	for (i=0; i<MAX_SUBTASKS; i++) {
		if (l4_task_equal(clnt->subtasks[i], L4_INVALID_ID)) {
			clnt->subtasks[i] = task;
			return 0;
		}
	}

	return -1;
}


/** Remove task from client's subtasks.
 *
 * Remove task from the subtasks belonging to client clnt, if they exist in
 * the client's list of subtasks.
 */
static void remove_task_from_client(client_t *clnt, l4_taskid_t task)
{
	int i;
	
	for (i=0; i<MAX_SUBTASKS; i++) {
		if (l4_task_equal(clnt->subtasks[i], task)) {
			clnt->subtasks[i] = L4_INVALID_ID;
		}
	}
}

/******************************************************************************
 * Server thread function                                                     *
 ******************************************************************************/

void serverthread(void *arg);
void serverthread(void *arg)
{
	int i, j;

	/* Empty client list. */
	for (i=0; i<MAX_CLIENTS; i++) {
		children[i].id = L4_INVALID_ID;
		for (j=0; j<MAX_SUBTASKS; j++)
			children[i].subtasks[j] = L4_INVALID_ID;
	}

	l4thread_started(NULL);
	l4_ts_server_loop(NULL);
}

void taskserverlib_init(void);
void taskserverlib_init(void)
{
	l4thread_t t;

	t = l4thread_create_named(serverthread, ".tasksrv", NULL,
	                          L4THREAD_CREATE_SYNC);
	l4task_server_thread = l4thread_l4_id(t);
}
L4C_CTOR(taskserverlib_init, L4CTOR_BEFORE_BACKEND);

void dice_server_error(l4_msgdope_t m, CORBA_Server_Environment *e)
{
	/* Client killed? Must have been an exit() */
	if (m.msgdope == L4_IPC_ENOT_EXISTENT)
		return;

	LOG("IPC error: %lx", m.msgdope);
	LOG("Dice exception: %x", DICE_EXCEPTION_MAJOR(e));
}

/******************************************************************************
 * Component functions.                                                       *
 ******************************************************************************/


long
l4_ts_allocate_component (CORBA_Object _dice_corba_obj,
                          unsigned long taskno,
                          l4_taskid_t *taskid,
                          CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	char status;
	client_t *c = NULL;

	err = allocate_callback(*_dice_corba_obj, taskno, taskid, &status);
	if (status == CALLBACK_SKIP)
		return err;

	/* Find client. */
	c = find_client(*_dice_corba_obj);

	/* Not found? Add it. */
	if (!c)
		c = add_client(*_dice_corba_obj);

	/* Not added? Out of mem. */
	if (!c)
		return -L4_ENOMEM;

	/* Allocate task and add it to client's subtasks. */
	err = l4ts_allocate_task(taskno, taskid);
	if (err == 0) {
		/* Make client the task's new chief. */
		l4_threadid_t t = l4_task_new(*taskid, _dice_corba_obj->raw,
		                              0, 0, L4_NIL_ID);
		if (l4_is_invalid_id(t))
			return -L4_EBADF;
		*taskid = t;
		LOGd(DEBUG, "transferred ownership of task "l4util_idfmt" to %X",
			l4util_idstr(*taskid), _dice_corba_obj->id.task);
		add_task_for_client(c, *taskid);
	}
	else
		return -L4_ENOTFOUND;

	post_allocate_callback(*_dice_corba_obj, taskid);

	return 0;
}


long
l4_ts_free_component (CORBA_Object _dice_corba_obj,
                      const l4_taskid_t *taskid,
                      CORBA_Server_Environment *_dice_corba_env)
{
	client_t *c = NULL;
	int err;
	char status;

	err = free_callback(*_dice_corba_obj, taskid, &status);
	if (status == CALLBACK_SKIP)
		return err;

	c = find_client(*_dice_corba_obj);

	/* Not found? Drop request. */
	if (!c)
		return -L4_EBADF;
	
	remove_task_from_client(c, *taskid);
	
	return l4ts_free_task(taskid);
}


static int terminate_task(l4_taskid_t tsk)
{
	client_t *c = find_client(tsk);
	l4_threadid_t t;
	int i;

	/* If we did not find a valid client above, this means one of two things:
	 * - client is not our client
	 * - client didn't allocate tasks and we don't need to clean up.
	 *
	 * We now try to exit the client. If this works, we are done, otherwise
	 * the exit() request was invalid.
	 */

	LOGd(DEBUG, "Terminating task "l4util_idfmt" (and all subtasks).", l4util_idstr(tsk));
	/* DIE! DIE! DIE! */
	t = l4_task_new(tsk, (unsigned)l4_myself().raw, 0, 0, L4_NIL_ID);
	LOGd(DEBUG, "Done: "l4util_idfmt, l4util_idstr(t));
	if (l4_is_invalid_id(t)) {
		LOG_Error("could not eliminate task.");
		return -L4_EBADF;
	}

	/* Killed client, no cleanup necessary. */
	if (!c)
		return 0;

	/* Now that the task has been killed, the kernel has also made us to be
	 * the chief of all subtasks. Now iterate through these subtasks and return
	 * them to our parent.
	 */
	for (i=0; i<MAX_SUBTASKS; i++) {
		l4_taskid_t tsk = c->subtasks[i];
		if (!l4_is_invalid_id(tsk)) {
			int r = l4ts_free_task(&tsk);
			if (r) {
				LOG("Error freeing task "l4util_idfmt,
					l4util_idstr(tsk));
				enter_kdebug();
			}
			remove_task_from_client(c, tsk);
		}
	}

	return 0;
}

long
l4_ts_exit_component (CORBA_Object _dice_corba_obj,
                      short *_dice_reply,
                      CORBA_Server_Environment *_dice_corba_env)
{
	int err;
	char status;
	
	err = exit_callback(*_dice_corba_obj, &status);
	if (status == CALLBACK_SKIP)
		return err;
	
	err = terminate_task(*_dice_corba_obj);
	
	/* We don't reply to valid exit requests. */
	if (!err)
		*_dice_reply = DICE_NO_REPLY;

	return err;
}


void
l4_ts_dump_component (CORBA_Object _dice_corba_obj,
                      CORBA_Server_Environment *_dice_corba_env)
{
	LOG_Enter();
}

