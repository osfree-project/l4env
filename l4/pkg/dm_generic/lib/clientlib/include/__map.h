/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/include/__map.h
 * \brief  internal use
 *
 * \date   05/12/2005
 * \author Frank Mehnert <fm3@os.inf.tu.dresden.de>
 */
/*****************************************************************************/

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_GENERIC___MAP_H
#define _DM_GENERIC___MAP_H

int
__do_map(const l4dm_dataspace_t *ds, l4_offs_t offs, l4_size_t size,
	 l4_addr_t rcv_addr, int rcv_size2, l4_offs_t rcv_offs, 
	 l4_uint32_t flags, l4_addr_t *fpage_addr, l4_size_t *fpage_size);

#endif
