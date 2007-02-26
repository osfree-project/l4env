/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__config.h
 * \brief  Library configuration.
 *
 * \date   08/29/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___CONFIG_H
#define _THREAD___CONFIG_H

/*****************************************************************************
 *** default values for the runtime configuration, see config.c
 *****************************************************************************/

/// max. number of threads 
#define L4THREAD_MAX_THREADS         16

/*****************************************************************************
 *** stack management
 *****************************************************************************/

/// default stack size
#define L4THREAD_STACK_SIZE          65536

/// max. stack size
#define L4THREAD_MAX_STACK_SIZE      0x00100000  /* 1 MB */

/// size of thread exit stack (see exit.c)
#define L4THREAD_EXIT_STACK_SIZE     8192

/*****************************************************************************
 *** priorities
 *****************************************************************************/

/**
 * Should we always call l4thread_schedule to get the priority of a thread 
 * or can we just use the value stored in the tcb of the thread. 
 *
 * If the priorities of threads are modified by other ways than the functions
 * provided by the thread lib, we do must call l4thread_schedule every time 
 * we need to get the priority of a thread, otherwise we just can use the 
 * value stored in the tcb.
 */
#define L4THREAD_PRIO_CALL_SCHEDULE  1

/*****************************************************************************
 *** thread data
 *****************************************************************************/

/**
 * max. number of thread data keys
 *
 * \note The current implementation uses a 32 bit bitfield for the key 
 * allocation, so the max. number must be lower than 32.
 */
#define L4THREAD_MAX_DATA_KEYS       8

#endif /* !_THREAD___CONFIG_H */
