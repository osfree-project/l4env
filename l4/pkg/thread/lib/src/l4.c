/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/l4.c
 * \brief  Architecture dependent functions, native L4 versions
 *
 * \date   04/12/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/sys/compiler.h>

/* library includes */
#include "__thread.h"

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Architecture dependent init
 */
/*****************************************************************************/ 
int 
l4th_init_arch(void)
{
  /* nothing to do in native L4 version */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Get pager for new threads
 *	
 * \return Pager thread id
 */
/*****************************************************************************/ 
l4_threadid_t 
l4th_thread_get_pager(void)
{
  /* thread pager -> region mapper thread */
  return l4rm_region_mapper_id();
}

/*****************************************************************************/
/**
 * \brief  Setup thread stack
 * 
 * \param  addr          Stack address 
 * \param  size          Stack size
 * \param  thread        L4 thread id
 * \param  flags         Flags
 *	
 * \return Stack pointer to be used to start new thread
 *
 * Setup thread and return initial stack pointer.
 */
/*****************************************************************************/ 
l4_addr_t
l4th_thread_setup_stack(l4_addr_t addr, l4_size_t size, l4_threadid_t thread,
			l4_uint32_t flags)
{
  /* nothing else necessary, return stack pointer */
  return addr + size;
}

/*****************************************************************************/
/**
 * \brief  Entry point for new threads
 *
 * This function cannot be instrumented, as it has no upper stack (caller).
 * Call-graph analysis would raise pagefaults.
 */
/*****************************************************************************/ 
void
l4th_thread_entry(void) L4_NOINSTRUMENT;
void
l4th_thread_entry(void)
{
  /* nothing special to do in native L4 version, just start the thread */
  l4th_thread_start();
}
