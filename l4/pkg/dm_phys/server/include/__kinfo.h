/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__kinfo.h
 * \brief  L4 kernel info page handling
 *
 * \date   02/05/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS___KINFO_H
#define _DM_PHYS___KINFO_H

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* return min. memory address */
l4_addr_t
dmphys_kinfo_mem_low(void);

/* return max. memory address */
l4_addr_t
dmphys_kinfo_mem_high(void);

/* initialise ram_base */
void
dmphys_kinfo_init_ram_base(void);

/* ram_base variable */
extern l4_addr_t ram_base;

#endif /* !_DM_PHYS___KINFO_H */
