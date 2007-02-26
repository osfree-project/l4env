/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__alloc.h
 * \brief  Memory allocation.
 *
 * \date   07/31/2001
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
#ifndef _L4RM___ALLOC_H
#define _L4RM___ALLOC_H

/* L4RM includes */
#include <l4/l4rm/l4rm.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

int
l4rm_heap_init(int have_l4env, 
	       l4rm_vm_range_t used[], 
	       int num_used);

int
l4rm_heap_register(void);

void *
l4rm_heap_alloc(void);

void
l4rm_heap_add_client(l4_threadid_t client);

void
l4rm_heap_remove_client(l4_threadid_t client);

#endif /* !_L4RM___ALLOC_H */
