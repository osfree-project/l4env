/* $Id$ */
/*****************************************************************************/
/**
 * \file    generic_ts/include/generic_ts.h
 * \brief   Generic task server API
 * \ingroup api
 *
 * \date    04/2004
 * \author  Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _GENERIC_TS_GENERIC_TS_H
#define _GENERIC_TS_GENERIC_TS_H

#include <l4/env/cdefs.h>
#include <l4/sys/types.h>

extern l4_threadid_t l4ts_server_id;

__BEGIN_DECLS

/**
 * \brief Are we connected to the task server?
 * \return		0 on success
 * 			-L4_ENOTFOUND	server not found. */
L4_INLINE int
l4ts_connected(void);

/**
 * \brief Force connecting to task server.
 * \return		0 on success
 * 			-L4_ENOTFOUND	server not found. */
int
l4ts_connect(void);

/**
 * \brief Allocate a task ID.
 * \retval taskid	allocated task ID.
 * \return		0 on success
 * 			error code otherwise. */
int
l4ts_allocate_task(l4_taskid_t *taskid);

/**
 * \brief Start a previously started task.
 * \param taskid	ID of the previos allocated task
 * \param entry		Initial instruction pointer
 * \param stack		Initial stack pointer
 * \param mcp		Maximum controlled priority (see L4-Spec)
 * \param pager		Pager of first thread
 * \param prio		Priority of first thread
 * \param resname	Module name as specified in the RMGR (subject of
 * 			future changes)
 * \param flags		(unused) */
int
l4ts_create_task(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
		 l4_uint32_t mcp, const l4_taskid_t *pager, l4_int32_t prio,
		 const char *resname, l4_uint32_t flags);

/**
 * \brief Free a task number.
 * \param taskid	ID of the task to free. */
int
l4ts_free_task(const l4_taskid_t *taskid);


#define	L4TS_KILL_SYNC	1 /* wait for freeing resources */
#define L4TS_KILL_FREE	2 /* decallocate task resource */
#define L4TS_KILL_NOEV	4 /* prevent sending the exit event (e.g. task
			     did not allocate any ressource) */

/**
 * \brief Delete a task.
 * \param taskid	ID of the task to kill. 
 * \param options */
int
l4ts_kill_task(l4_taskid_t taskid, l4_uint8_t options);

/**
 * \brief Delete a task.
 * \param taskid	ID of the task to kill. */
int
l4ts_kill_task_recursive(l4_taskid_t taskid);

/**
 * \brief Convert a task number to a task ID.
 * \param taskno	Corresponding task ID. */
int
l4ts_owner(l4_taskid_t taskid, l4_taskid_t owner);

/**
 * \brief Convert a task number to a task ID.
 * \param taskno	Corresponding task ID. */
int
l4ts_taskno_to_taskid(l4_uint32_t taskno, l4_taskid_t *taskid);

/**
 * \brief Exit the caller.
 */
int
l4ts_exit(void) __attribute__((noreturn));

/**
 * \brief Dump all registered tasks.
 */
int
l4ts_dump_tasks(void);


L4_INLINE int
l4ts_connected(void)
{
  if (EXPECT_FALSE(l4_is_nil_id(l4ts_server_id)))
    {
      if (l4ts_connect() != 0)
	return 0;
    }
  return 1;
}

__END_DECLS

#endif
