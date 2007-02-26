/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__libl4rm.h
 * \brief  Private lib interfaces
 *
 * \date   06/03/2000 
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
#ifndef _L4RM___LIBL4RM_H
#define _L4RM___LIBL4RM_H

/* L4 includes */
#include <l4/sys/types.h>

/// service loop thread id
extern l4_threadid_t l4rm_service_id;

/*****************************************************************************
 * prototypes
 *****************************************************************************/

void
l4rm_handle_pagefault(l4_threadid_t src_id, 
		      l4_addr_t addr,
		      l4_addr_t eip);

#endif /* !_L4RM___LIBL4RM_H */
