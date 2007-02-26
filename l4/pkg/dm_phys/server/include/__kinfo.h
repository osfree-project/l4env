/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__kinfo.h
 * \brief  L4 kernel info page handling
 *
 * \date   02/05/2002
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
#ifndef _DM_PHYS___KINFO_H
#define _DM_PHYS___KINFO_H

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* return max. memory address */
l4_addr_t
dmphys_kinfo_mem_high(void);

#endif /* !_DM_PHYS___KINFO_H */
