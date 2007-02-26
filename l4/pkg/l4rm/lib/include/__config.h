/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__config.h
 * \brief  Region mapper library config.
 *
 * \date   05/27/2000
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
#ifndef _L4RM___CONFIG_H
#define _L4RM___CONFIG_H

/* L4/L4Env includes */
#include <l4/sys/types.h>

/*****************************************************************************
 *** Memory allocation
 *****************************************************************************/

/**
 * Max. size of region descriptor / AVL tree node heap
 */
#define L4RM_MAX_HEAP_SIZE        (512 * 1024)

/**
 * Initial size of heap dataspace 
 */
#define L4RM_HEAP_DS_INIT_PAGES   2

/*****************************************************************************
 *** Regions
 *****************************************************************************/

/** 
 * Default start address of virtual memory, it is used if no vm start address
 * is set by L4Env (do not include page 0!)
 */
#define L4RM_VM_START             0x00001000

/** 
 * Default end address of virtual memory
 */
#define L4RM_VM_END               0xC0000000

/**
 * Default region area id
 */
#define L4RM_DEFAULT_REGION_AREA  0

/*****************************************************************************
 *** Service loop
 *****************************************************************************/

/// first valid request id
#define L4RM_FIRST_REQUEST        0xC0000000

/*****************************************************************************
 *** Pagefaul handling
 *****************************************************************************/

/// allow forwarding of unknown pagefaults to default pager
#define L4RM_FORWARD_PAGEFAULTS         1

/// forward pagefaults by generating an explicit IPC message
#define L4RM_FORWARD_PAGEFAULTS_BY_IPC  1

/*****************************************************************************
 *** Prototypes 
 *****************************************************************************/

l4_addr_t
l4rm_get_vm_start(void);

l4_addr_t
l4rm_get_vm_end(void);

l4_threadid_t 
l4rm_get_dsm(void);

l4_threadid_t
l4rm_get_sigma0(void);

#endif /* !_L4RM___CONFIG_H */
