/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__sigma0.h
 * \brief  Sigma0 communication
 *
 * \date   08/04/2001
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
#ifndef _DM_PHYS___SIGMA0_H
#define _DM_PHYS___SIGMA0_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/* init sigma0 comm */
int
dmhys_sigma0_init(void);

/* map some page */
void *
dmphys_sigma0_map_any_page(void);

/* map a specific page */
int
dmphys_sigma0_map_page(l4_addr_t page);

/* unmap a page */
void
dmphys_sigma0_unmap_page(l4_addr_t page);

/* map a 4M-page */
int
dmphys_sigma0_map_4Mpage(l4_addr_t page);

/* unmap a 4M-page */
void
dmphys_sigma0_unmap_4Mpage(l4_addr_t page);

/* map L4 kinfo page */
l4_kernel_info_t *
dmphys_sigma0_kinfo(void);

#endif /* !_DM_PHYS___SIGMA0_H */
