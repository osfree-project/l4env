/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__alloc.h
 * \brief  Memory allocation.
 *
 * \date   07/31/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4RM___ALLOC_H
#define _L4RM___ALLOC_H

/* L4RM includes */
#include <l4/l4rm/l4rm.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

int
l4rm_heap_init(int have_l4env, l4rm_vm_range_t used[], int num_used);

int
l4rm_heap_register(void);

void *
l4rm_heap_alloc(void);

#endif /* !_L4RM___ALLOC_H */
