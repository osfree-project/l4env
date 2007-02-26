/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_phys/include/consts.h
 * \brief   DMphys, public constants
 * \ingroup api
 *
 * \date    11/23/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS_CONSTS_H
#define _DM_PHYS_CONSTS_H

/* DMphys nameserver name */
#define L4DM_MEMPHYS_NAME        "DM_PHYS"

/* memory pools (see server/include/__config.h) */
#define L4DM_MEMPHYS_DEFAULT     0   /**< \ingroup api_open
				      **  Use default memory pool
				      **/
#define L4DM_MEMPHYS_ISA_DMA     7   /**< \ingroup api_open
				      **  Use ISA DMA capable memory
				      **  (below 16MB)
				      **/

/* open arguments */
#define L4DM_MEMPHYS_ANY_ADDR    -1  /**< \ingroup api_open
				      **  find an appropriate memory area
				      **/

/* Flags, see l4/dm_generic/consts.h for the definition of the
 * flag bit mask
 */
#define L4DM_MEMPHYS_SUPERPAGES  0x00010000   /**< \ingroup api_open
					       **  Open/map: force super-pages
					       **/
#define L4DM_MEMPHYS_SAME_POOL   0x00020000   /**< \ingroup api_open
					       **  Copy: use same pool like
					       **  source to allocate memory
					       **/

/* debug keys */
#define L4DM_MEMPHYS_SHOW_MEMMAP       0x00000001
#define L4DM_MEMPHYS_SHOW_POOLS        0x00000002
#define L4DM_MEMPHYS_SHOW_POOL_AREAS   0x00000003
#define L4DM_MEMPHYS_SHOW_POOL_FREE    0x00000004
#define L4DM_MEMPHYS_SHOW_SLABS        0x00000005

#endif /* !_DM_PHYS_CONSTS_H */
