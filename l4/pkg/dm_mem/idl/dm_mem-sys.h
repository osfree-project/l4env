/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_mem/idl/dm_mem-sys.h
 * \brief   Memory dataspace manager interface, function numbers
 * \ingroup idl_mem
 *
 * \date    11/19/2001
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
#ifndef _DM_MEM_SYS_H
#define _DM_MEM_SYS_H

#include <l4/dm_generic/dm_generic-sys.h>

#define req_if_l4dm_mem_open                 0x0101
#define req_if_l4dm_mem_size                 0x0102
#define req_if_l4dm_mem_resize               0x0103
#define req_if_l4dm_mem_phys_addr            0x0104
#define req_if_l4dm_mem_is_contiguous        0x0105
#define req_if_l4dm_mem_lock                 0x0106
#define req_if_l4dm_mem_unlock               0x0107

#endif /* !_DM_MEM_SYS_H */
