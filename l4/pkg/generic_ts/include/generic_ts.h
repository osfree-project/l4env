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
 *			-L4_ENOTFOUND	server not found. */
L4_INLINE int
l4ts_connected(void);

/**
 * \brief Force connecting to task server.
 * \return		0 on success
 *			-L4_ENOTFOUND	server not found. */
L4_CV int
l4ts_connect(void);

/**
 * \brief Allocate a task ID.
 * \param  taskno       a dedicated task ID is required if != 0
 * \retval taskid	allocated task ID.
 * \return		0 on success
 *			error code otherwise. */
L4_CV int
l4ts_allocate_task(unsigned int taskno, l4_taskid_t *taskid);

/**
 * \brief Allocate a task ID and become the task's chief.
 * \param  taskno       a dedicated task ID is required if != 0
 * \retval taskid       allocated task ID
 * \return              0 on success
 *                      error code otherwise
 */
L4_CV int
l4ts_allocate_task2(unsigned int taskno, l4_taskid_t *taskid);

/**
 * \brief Start a previously allocated task.
 *
 * \param taskid	ID of the previos allocated task
 * \param entry		Initial instruction pointer
 * \param stack		Initial stack pointer
 * \param mcp		Maximum controlled priority (see L4-Spec)
 * \param pager		Pager of first thread
 * \param prio		Priority of first thread
 * \param resname	Module name as specified in the RMGR (subject of
 *			future changes)
 * \param flags		(currently unused)
 */
L4_CV int
l4ts_create_task(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
                 l4_uint32_t mcp, const l4_taskid_t *pager, l4_int32_t prio,
                 const char *resname, l4_uint32_t flags);

/**
 * \brief Start a previously allocated task, long version.
 *
 * \param taskid       ID of the previos allocated task
 * \param entry        Initial instruction pointer
 * \param stack        Initial stack pointer
 * \param mcp          Maximum controlled priority (see L4-Spec)
 * \param pager        Pager of first thread
 * \param caphandler   The task's capability handler
 * \param kquota       In-kernel quota of the task
 * \param prio         Priority of first thread
 * \param resname      Module name as specified in the RMGR (subject of
 *                     future changes)
 * \param flags        (currently unused)
 */
L4_CV int
l4ts_create_task2(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
                  l4_uint32_t mcp, const l4_taskid_t *pager,
                  const l4_taskid_t *caphandler, l4_quota_desc_t kquota,
                  l4_int32_t prio, const char *resname, l4_uint32_t flags);

/**
 * \brief Free a task number.
 * \param taskid	ID of the task to free. */
L4_CV int
l4ts_free_task(const l4_taskid_t *taskid);

/**
 * \brief Free a task number and return chief rights.
 * \param taskid	ID of the task to free. */
L4_CV int
l4ts_free2_task(const l4_taskid_t *taskid);


#define	L4TS_KILL_SYNC	1 /* wait for freeing resources */
#define L4TS_KILL_FREE	2 /* decallocate task resource */
#define L4TS_KILL_NOEV	4 /* prevent sending the exit event (e.g. task
			     did not allocate any ressource) */

/**
 * \brief Delete a task.
 * \param taskid	ID of the task to kill.
 * \param options */
L4_CV int
l4ts_kill_task(l4_taskid_t taskid, l4_uint8_t options);

/**
 * \brief Delete a task.
 * \param taskid	ID of the task to kill. */
L4_CV int
l4ts_kill_task_recursive(l4_taskid_t taskid);

/**
 * \brief Convert a task number to a task ID.
 * \param taskno	Corresponding task ID. */
L4_CV int
l4ts_owner(l4_taskid_t taskid, l4_taskid_t owner);

/**
 * \brief Convert a task number to a task ID.
 * \param taskno	Corresponding task ID. */
L4_CV int
l4ts_taskno_to_taskid(l4_uint32_t taskno, l4_taskid_t *taskid);

/**
 * \brief Exit the caller.
 */
L4_CV int
l4ts_exit(void) __attribute__((noreturn));

/**
 * \brief Dump all registered tasks.
 */
L4_CV int
l4ts_dump_tasks(void);

/**
 * \brief Return server ID.
 */
L4_CV l4_threadid_t
l4ts_server(void);

L4_INLINE int
l4ts_connected(void)
{
  if (EXPECT_FALSE(l4_is_nil_id(l4ts_server())))
    {
      if (l4ts_connect() != 0)
	return 0;
    }
  return 1;
}

__END_DECLS

#endif
