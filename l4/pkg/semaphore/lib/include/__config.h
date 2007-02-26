/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/lib/include/__config.h
 * \brief  Library configuration.
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _SEMAPHORE___CONFIG_H
#define _SEMAPHORE___CONFIG_H

/**
 * Semaphore thread priority
 */
#define L4SEMAPHORE_THREAD_PRIO       255

/**
 * Semaphore thread stack size 
 */
#define L4SEMAPHORE_THREAD_STACK_SIZE 4096

/**
 * Max. number of wait queue entries
 */
#define L4SEMAPHORE_MAX_WQ_ENTRIES    256

/**
 * Priority sort semaphore wait queues
 */ 
#define L4SEMAPHORE_SORT_WQ           1

#endif /* !_SEMAPHORE___CONFIG_H */
