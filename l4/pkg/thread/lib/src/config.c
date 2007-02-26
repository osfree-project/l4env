/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/config.c
 * \brief  Thread lib configuration
 *
 * \date   03/19/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>

/* l4thread includes */
#include <l4/thread/thread.h>
#include "__config.h"

/*****************************************************************************
 *** global configuration data, they can be overwritten by user symbols
 *****************************************************************************/

/// max. number of threads
const int l4thread_max_threads __attribute__ ((weak)) 
  = L4THREAD_MAX_THREADS;

/// default stack size
const l4_size_t l4thread_stack_size __attribute__ ((weak)) 
  = L4THREAD_STACK_SIZE;

/// max. stack size
const l4_size_t l4thread_max_stack __attribute__ ((weak)) 
  = L4THREAD_MAX_STACK_SIZE;

/// default priority
l4_prio_t l4thread_default_prio __attribute__ ((weak)) 
  = L4THREAD_DEFAULT_PRIO;

/// stack map area start address, if set to -1 a suitable address is used
const l4_addr_t l4thread_stack_area_addr __attribute__ ((weak)) = -1;

/// TCB table map address, if set to -1 a suitable address is used
const l4_addr_t l4thread_tcb_table_addr __attribute__ ((weak)) = -1;
