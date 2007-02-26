/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/include/__config.h
 * \brief  Generic dataspace manager client library configuration
 *
 * \date   01/31/2002
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
#ifndef _DM_MEM___CONFIG_H
#define _DM_MEM___CONFIG_H

/* L4 includes */
#include <l4/sys/types.h>

/*****************************************************************************
 *** default values for l4dm_allocate*
 *****************************************************************************/

/**
 * default alignment
 */
#define DMMEM_ALLOCATE_ALIGN  (L4_PAGESIZE)

#endif /* !_DM_MEM___CONFIG_H */
