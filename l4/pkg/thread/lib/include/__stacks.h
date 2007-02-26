/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__stacks.h
 * \brief  Stack handling.
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___STACKS_H
#define _THREAD___STACKS_H

/* L4/L4Env includes */
#include <l4/env/cdefs.h>

/* library includes */
#include "__memory.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

extern int       l4th_have_stack_area;
extern l4_addr_t l4th_stack_area_start;  ///< start address of the stack area
extern l4_addr_t l4th_stack_area_end;    ///< end address of the stack area

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

int
l4th_stack_init(void);

int
l4th_stack_allocate(int index, l4_size_t size, l4_uint32_t flags,
		    l4_threadid_t owner, l4th_mem_desc_t * desc);

void
l4th_stack_free(l4th_mem_desc_t * desc);

L4_INLINE int
l4th_stack_get_current_id(void);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

#ifdef THREAD_L4
#  include "__stacks_l4.h"
#endif

#ifdef THREAD_LINUX
#  include "__stacks_linux.h"
#endif

#ifdef THREAD_LINUX_KERNEL
#  include "__stacks_linux_kernel.h"
#endif

#endif /* !_THREAD___STACKS_H */
