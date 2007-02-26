/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_phys/idl/dm_phys-sys.h
 * \brief   Phys. memory dataspace manager interface, function numbers
 * \ingroup idl_phys
 * 
 * \date    11/24/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
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
#ifndef _DM_PHYS_SYS_H
#define _DM_PHYS_SYS_H

#include <l4/dm_generic/dm_generic-sys.h>
#include <l4/dm_mem/dm_mem-sys.h>

#define req_if_l4dm_memphys_dmphys_open      0x0201
#define req_if_l4dm_memphys_dmphys_copy      0x0202
#define req_if_l4dm_memphys_dmphys_pagesize  0x0203
#define req_if_l4dm_memphys_dmphys_debug     0x0204

#endif /* !_DM_PHYS_SYS_H */
