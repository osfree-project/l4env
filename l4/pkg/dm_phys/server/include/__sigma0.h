/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__sigma0.h
 * \brief  Sigma0 communication
 *
 * \date   08/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
