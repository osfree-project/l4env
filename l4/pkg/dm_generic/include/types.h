/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/include/types.h
 * \brief  Generic dataspace manager interface, type definitions
 *
 * \date   11/12/2002
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
#ifndef _L4_DM_GENERIC_TYPES_H
#define _L4_DM_GENERIC_TYPES_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/*****************************************************************************
 *** Types
 *****************************************************************************/

/**
 * L4 dataspace id
 * \ingroup types
 */
typedef struct l4dm_dataspace
{
  l4_uint32_t   id;
  l4_threadid_t manager;
} l4dm_dataspace_t;

/**
 * Invalid dataspace id initializer
 * \ingroup types
 */
#define L4DM_INVALID_DATASPACE_INITIALIZER  { (l4_uint32_t)-1, L4_INVALID_ID}
  
/**
 * Invalid dataspace id
 * \ingroup types
 */
#define L4DM_INVALID_DATASPACE \
		((l4dm_dataspace_t)L4DM_INVALID_DATASPACE_INITIALIZER)

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief Test dataspace ids
 * \ingroup types
 *	
 * \param  ds1           First dataspace id
 * \param  ds2           Second dataspaces id
 *
 * \return 1 if dataspaces ids are equal, 0 otherwise.
 */
/*****************************************************************************/ 
L4_INLINE int
l4dm_dataspace_equal(l4dm_dataspace_t ds1, 
		     l4dm_dataspace_t ds2);

/*****************************************************************************/
/**
 * \brief  Test dataspace id
 * \ingroup types
 * 
 * \param  ds            Dataspace id
 *	 
 * \return != 0 if ds is invalid dataspace descriptor, 0 if not
 */
/*****************************************************************************/ 
L4_INLINE int
l4dm_is_invalid_ds(l4dm_dataspace_t ds);

__END_DECLS;

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/* l4dm_dataspace_equal */
L4_INLINE int
l4dm_dataspace_equal(l4dm_dataspace_t ds1, 
		     l4dm_dataspace_t ds2)
{
  if (l4_thread_equal(ds1.manager,ds2.manager) && (ds1.id == ds2.id))
    return 1;
  else
    return 0;
}

/* l4dm_is_invalid_ds */
L4_INLINE int
l4dm_is_invalid_ds(l4dm_dataspace_t ds)
{
  return ((ds.id == L4DM_INVALID_DATASPACE.id) &&
	  (l4_thread_equal(ds.manager,L4DM_INVALID_DATASPACE.manager)));
}

#endif /* !_L4_DM_GENERIC_TYPES_H */
