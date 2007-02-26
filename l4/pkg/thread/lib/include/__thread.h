/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__thread.h
 * \brief  Internal thread lib prototypes
 *
 * \date   04/12/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
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
