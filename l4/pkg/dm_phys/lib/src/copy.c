/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/copy.c
 * \brief  DMphys client library, copy dataspace
 *
 * \date   02/03/2002
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

/* DMphys includes */
#include <l4/dm_phys/consts.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_phys/dm_phys-client.h>
#include "__debug.h"

/*****************************************************************************
 *** DMphys client lib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create dataspace copy
 * 
 * \param  ds            Source dataspace id
 * \param  src_offs      Offset in source dataspace
 * \param  dst_offs      Offset in destination dataspace
 * \param  num           Number of bytes to copy, set to L4DM_WHOLE_DS to copy 
 *                       the whole dataspace starting at \a src_offs
 * \param  dst_pool      Memory pool to use to allocate destination dataspace
 * \param  dst_addr      Phys. address of destination dataspace, set to 
 *                       L4DM_MEMPHYS_ANY_ADDR to find an appropriate address
 * \param  dst_size      Size of destination dataspace, if larger than 
 *                       \a dst_offs + \a num it is used as the size of the
 *                       destination dataspace
 * \param  dst_align     Alignment of destination dataspace
 * \param  flags         Flags:
 *                       - \c L4DM_CONTIGUOUS        create copy on phys. 
 *                                                   contiguos memory
 *                       - \c L4DM_MEMPHYS_SAME_POOL use same memory pool like
 *                                                   source to allocate 
 *                                                   destination dataspace
 * \param  name          Destination dataspace name
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
l4dm_memphys_copy(l4dm_dataspace_t * ds, 
		  l4_offs_t src_offs, 
		  l4_offs_t dst_offs,
		  l4_size_t num, 
		  int dst_pool, 
		  l4_addr_t dst_addr, 
		  l4_size_t dst_size, 
		  l4_addr_t dst_align, 
		  l4_uint32_t flags, 
		  const char * name, 
		  l4dm_dataspace_t * copy)
{
  l4_threadid_t dsm_id;
  int ret;
  sm_exc_t _exc;

  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    return -L4_ENODM;

  if ((ds == NULL) || !l4_thread_equal(dsm_id,ds->manager))
    return -L4_EINVAL;

  /* call DMphys */
  ret = if_l4dm_memphys_dmphys_copy(dsm_id,ds->id,src_offs,dst_offs,num,
				    dst_pool,dst_addr,dst_size,dst_align,
				    flags,name,(if_l4dm_dataspace_t *)copy,
				    &_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      ERROR("libdm_phys: copy dataspace %u at DMphys (%x.%x) failed "
	    "(ret %d, exc %d)",ds->id,dsm_id.id.task,dsm_id.id.lthread,
	    ret,_exc._type);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }
  
  /* done */
  return ret;
}
