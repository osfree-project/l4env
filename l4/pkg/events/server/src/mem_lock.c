/*!
 * \file   events/server/src/mem_lock.c
 *
 * \brief  Memory lock implementation for the event server. We don't need
 *         any memory lock since we are single threaded. We therefore don't
 *         need the ugly implementation from oskit_support which needs
 *         cli/sti.
 *
 * \date   06/2004
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

void __mem_lock(void);
void __mem_unlock(void);

void
__mem_lock(void)
{
}

void
__mem_unlock(void)
{
}
