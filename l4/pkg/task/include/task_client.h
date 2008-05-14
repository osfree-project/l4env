#ifndef __GUARD_TASK_CLIENT_H
#define __GUARD_TASK_CLIENT_H

/******************************************************************************
 * Extensions to the generic_ts interface provided by the hierarchical task   *
 * server libraries.                                                          *
 *                                                                            *
 * Bjoern Doebel <doebel@tudos.org>                                           *
 *                                                                            *
 * (c) 2007 Technische Universitaet Dresden                                   *
 * This file is part of DROPS, which is distributed under the terms of the    *
 * GNU General Public License 2. Please see the COPYING file for details.     *
 ******************************************************************************/

#include <l4/sys/linkage.h>

/** Callbacks available for task creators.
 *
 * Every callback function receives the parameters of the instrumented function
 * as well as a status value as parameter. The status value can be set by
 * the callback in order to indicate what should happen after the callback has
 * been executed. Possible status values are:
 *
 * CALLBACK_SKIP        - Skip execution of the rest of the instrumented function.
 *                        The callback's return value is returned.
 * CALLBACK_CONTINUE    - Continue execution of the instrumented function. The
 *                        function's return value is determined later.
 *
 * By default CALLBACK_CONTINUE is used.
 * */
typedef struct {
	L4_CV long (*client_alloc_callback)(unsigned int taskno, l4_taskid_t *task, char *status);
	L4_CV long (*client_free_callback)(const l4_taskid_t *task, char *status);
	L4_CV long (*client_exit_callback)(char *status);
	L4_CV long (*client_create_callback)(l4_taskid_t *task, l4_addr_t eip, l4_addr_t esp,
	                                     l4_uint32_t mcp, const l4_taskid_t *pager,
	                                     const l4_taskid_t *caphandler, l4_int32_t prio,
	                                     const char *name, l4_uint32_t flags, char *status);
	L4_CV long (*client_kill_callback)(l4_taskid_t *task, l4_uint8_t options, char *status);
} l4task_client_callbacks;

#define CALLBACK_SKIP		0   /**< callback success, skip calling server */
#define CALLBACK_CONTINUE	1   /**< callback success, but continue calling server */

/* Register a set of callback functions */
L4_CV int l4task_client_register_hooks(l4task_client_callbacks cb);

#endif
