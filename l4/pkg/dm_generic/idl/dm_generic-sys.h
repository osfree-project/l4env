/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_generic/idl/dm_generic-sys.h
 * \brief   Generic dataspace manager interface, function numbers
 * \ingroup idl_generic
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
#ifndef _DM_GENERIC_SYS_H
#define _DM_GENERIC_SYS_H

#define req_if_l4dm_generic_map              0x0001
#define req_if_l4dm_generic_fault            0x0002
#define req_if_l4dm_generic_close            0x0003
#define req_if_l4dm_generic_close_all        0x0004
#define req_if_l4dm_generic_share            0x0005
#define req_if_l4dm_generic_revoke           0x0006
#define req_if_l4dm_generic_check_rights     0x0007
#define req_if_l4dm_generic_transfer         0x0008
#define req_if_l4dm_generic_copy             0x0009
#define req_if_l4dm_generic_set_name         0x000A
#define req_if_l4dm_generic_get_name         0x000B
#define req_if_l4dm_generic_show_ds          0x000C
#define req_if_l4dm_generic_dump             0x000D
#define req_if_l4dm_generic_list             0x000E

#endif /* !_DM_GENERIC_SYS_H */
