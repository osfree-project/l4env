#ifndef __GUARD_TASK_H
#define __GUARD_TASK_H

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

#include <l4/sys/types.h>

typedef struct {
	long (*allocate_callback)(l4_threadid_t client, unsigned int taskno, l4_taskid_t *task, char *status);
	void (*post_alloc_callback)(l4_threadid_t client, l4_taskid_t *task);
	long (*free_callback)(l4_threadid_t client, const l4_taskid_t *task, char *status);
	long (*exit_callback)(l4_threadid_t client, char *status);
} l4task_server_callbacks;

#define CALLBACK_SKIP		0   /**< callback success, skip calling server */
#define CALLBACK_CONTINUE	1   /**< callback success, but continue calling server */

/** Get task server thread ID. */
l4_threadid_t l4task_get_server(void);

int l4task_server_register_hooks(l4task_server_callbacks cb);

#endif
