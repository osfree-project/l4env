/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/idl/l4rm-sys.h
 * \brief  IDL function numbers.
 *
 * \date   06/03/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 * 
 * \note The function numbers are defined above 0xC0000000, so the same thread
 *       can handle Flick requests and pagefaults (addresses above 0xC0000000
 *       are reserved for the L4 kernel)
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
#ifndef _L4RM_L4RM_SYS_H
#define _L4RM_L4RM_SYS_H

/* interface function numbers */
#define req_l4_rm_add                0xC0000000
#define req_l4_rm_remove             0xC0000001
#define req_l4_rm_lookup             0xC0000002
#define req_l4_rm_add_client         0xC0000003
#define req_l4_rm_remove_client      0xC0000004

#endif /* !_L4RM_L4RM_SYS_H */
