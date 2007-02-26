/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/release.c
 * \brief  Release area reserved with l4rm_area_reserve.
 *
 * \date   08/02/2001
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

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** L4RM client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Release area.
 * 
 * \param  area          Area id
 *	
 * \return 0 on success (area released), error code otherwise.
 *
 * Release area.
 */
/*****************************************************************************/ 
int
l4rm_area_release(l4_uint32_t area)
{
  /* lock region list */
  l4rm_lock_region_list();

  /* release area */
  l4rm_reset_area(area);

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Release area at given address
 * 
 * \param  ptr           VM address
 *	
 * \return 0 on success (area released), error code otherwise:
 *         - -#L4_EINVAL  invalid address, address belongs not to a reserved 
 *                        area
 */
/*****************************************************************************/ 
int
l4rm_area_release_addr(void * ptr)
{
  l4rm_region_desc_t * rp;

  /* lock region list */
  l4rm_lock_region_list();

  /* find region */
  rp = l4rm_find_region((l4_addr_t)ptr);
  
  if ((rp == NULL) || (REGION_AREA(rp) == L4RM_DEFAULT_REGION_AREA))
    return -L4_EINVAL;

  /* release area */
  l4rm_reset_area(REGION_AREA(rp));
  
  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return 0;
}
