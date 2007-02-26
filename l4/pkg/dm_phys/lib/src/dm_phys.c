/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/
 * \brief  DMphys client library, find DMphys
 *
 * \date   02/14/2002
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

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>

/* DMphys includes */
#include <l4/dm_phys/dm_phys.h>

/*****************************************************************************
 *** global variables
 *****************************************************************************/

/**
 * DMphys server thread id
 */
static l4_threadid_t dmphys_id = L4_INVALID_ID;

/*****************************************************************************/
/**
 * \brief  Find DMphys
 *	
 * \return DMphys id, L4_INVALID_ID if not found.
 */
/*****************************************************************************/ 
l4_threadid_t 
l4dm_memphys_find_dmphys(void)
{
  if (l4_is_invalid_id(dmphys_id))
    {
      if (!names_waitfor_name(L4DM_MEMPHYS_NAME,&dmphys_id,10000))
	{
	  ERROR("libdm_phys: DMphys not found!");
	  dmphys_id = L4_INVALID_ID;
	}
    }

  return dmphys_id;
}
