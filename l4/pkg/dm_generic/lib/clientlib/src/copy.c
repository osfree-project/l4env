/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/copy.c
 * \brief  Generic dataspace manager client library, copy dataspace
 *
 * \date   01/28/2002
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
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Create the copy.
 * 
 * \param  ds            Source dataspace id
 * \param  src_offs      Offset in source dataspace
 * \param  dst_offs      Offset in destination dataspace
 * \param  num           Number of bytes to copy
 * \param  flags         Flags
 * \param  name          Copy name
 * \retval copy          Dataspace id of copy
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC       IPC error calling dataspace manager
 *         - \c -L4_EINVAL     Invalid source dataspace id
 *         - \c -L4_EPERM      Permission denied
 *         - \c -L4_ENOHANDLE  Could not create dataspace descriptor
 *         - \c -L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
static int
__do_copy(l4dm_dataspace_t * ds, 
	  l4_offs_t src_offs, 
	  l4_offs_t dst_offs,
	  l4_size_t num, 
	  l4_uint32_t flags, 
	  const char * name, 
	  l4dm_dataspace_t * copy)
{
  int ret;
  sm_exc_t _exc;
  
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  if (name != NULL)
    ret = if_l4dm_generic_copy(ds->manager,ds->id,src_offs,dst_offs,num,
			       flags,name,(if_l4dm_dataspace_t *)copy,&_exc);
  else
    ret = if_l4dm_generic_copy(ds->manager,ds->id,src_offs,dst_offs,num,
			       flags,"",(if_l4dm_dataspace_t *)copy,&_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_generic: copy ds %u at %x.%x failed (ret %d, exc %d)",
	    ds->id,ds->manager.id.task,ds->manager.id.lthread,ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }
      
  /* done */
  return 0;
}

/*****************************************************************************
 *** libdm_generic API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create dataspace copy, short form
 * 
 * \param  ds            Source dataspace id
 * \param  flags         Flags:
 *                       - \c L4DM_COW         create copy-on-write copy
 *                       - \c L4DM_PINNED      create copy on pinned memory
 *                       - \c L4DM_CONTIGUOUS  create copy on phys. contiguos 
 *                                             memory
 * \param  name          Copy name
 * \retval copy          Copy dataspace id
 *	
 * \return 0 on success (\a copy contains the id of the created copy),
 *         error code otherwise:
 *         - \c -L4_EIPC       IPC error calling dataspace manager
 *         - \c -L4_EINVAL     Invalid source dataspace id
 *         - \c -L4_EPERM      Permission denied
 *         - \c -L4_ENOHANDLE  Could not create dataspace descriptor
 *         - \c -L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
int
l4dm_copy(l4dm_dataspace_t * ds, 
	  l4_uint32_t flags, 
	  const char * name, 
	  l4dm_dataspace_t * copy)
{
  /* create copy */
  return __do_copy(ds,0,0,L4DM_WHOLE_DS,flags,name,copy);
}

/*****************************************************************************/
/**
 * \brief  Create dataspace copy, long form
 * 
 * \param  ds            Source dataspace id
 * \param  src_offs      Offset in source dataspace
 * \param  dst_offs      Offset in destination dataspace
 * \param  num           Number of bytes to copy, set to L4DM_WHOLE_DS to copy 
 *                       the whole dataspace starting at \a src_offs
 * \param  flags         Flags
 *                       - \c L4DM_COW         create copy-on-write copy
 *                       - \c L4DM_PINNED      create copy on pinned memory
 *                       - \c L4DM_CONTIGUOUS  create copy on phys. contiguos 
 *                                             memory
 * \param  name          Copy name
 * \retval copy          Dataspace id of copy
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC       IPC error calling dataspace manager
 *         - \c -L4_EINVAL     Invalid source dataspace id
 *         - \c -L4_EPERM      Permission denied
 *         - \c -L4_ENOHANDLE  Could not create dataspace descriptor
 *         - \c -L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
int
l4dm_copy_long(l4dm_dataspace_t * ds, 
	       l4_offs_t src_offs, 
	       l4_offs_t dst_offs,
	       l4_size_t num, 
	       l4_uint32_t flags, 
	       const char * name, 
	       l4dm_dataspace_t * copy)
{
  /* create copy */
  return __do_copy(ds,src_offs,dst_offs,num,flags,name,copy);
}
