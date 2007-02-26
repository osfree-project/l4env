/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__thread.h
 * \brief  Internal thread lib prototypes
 *
 * \date   04/12/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___THREAD_H
#define _THREAD___THREAD_H

/* L4 includes */
#include <l4/sys/types.h>

/* library includes */
#include "__tcb.h"

/*****************************************************************************
 *** thread creation
 *****************************************************************************/

void
l4th_thread_start(void);

/*****************************************************************************
 *** thread exit
 *****************************************************************************/

void
l4th_return(void);

void
l4th_shutdown(l4th_tcb_t * tcb);

/*****************************************************************************
 *** architecture dependent functions
 *****************************************************************************/

int
l4th_init_arch(void);

l4_threadid_t 
l4th_thread_get_pager(void);

l4_addr_t
l4th_thread_setup_stack(l4_addr_t addr, l4_size_t size, l4_threadid_t thread,
			l4_uint32_t flags);

void
l4th_thread_entry(void);

#endif /* !_THREAD___THREAD_H */
