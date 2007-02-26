/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/include/__phys_addr.h
 * \brief  internal use
 *
 * \date   2006/01
 * \author Frank Mehnert <fm3@os.inf.tu.dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_MEM___PHYS_ADDR_H
#define _DM_MEM___PHYS_ADDR_H

int
__get_phys_addr(const l4dm_dataspace_t * ds, l4_offs_t offset, l4_size_t size,
		l4_addr_t * paddr, l4_size_t * psize);

#endif
