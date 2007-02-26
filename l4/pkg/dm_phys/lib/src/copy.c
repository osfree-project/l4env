/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/copy.c
 * \brief  DMphys client library, copy dataspace
 *
 * \date   02/03/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
 * \param  num           Number of bytes to copy, set to #L4DM_WHOLE_DS to copy 
 *                       the whole dataspace starting at \a src_offs
 * \param  dst_pool      Memory pool to use to allocate destination dataspace
 * \param  dst_addr      Phys. address of destination dataspace, set to 
 *                       #L4DM_MEMPHYS_ANY_ADDR to find an appropriate address
 * \param  dst_size      Size of destination dataspace, if larger than 
 *                       \a dst_offs + \a num it is used as the size of the
 *                       destination dataspace
 * \param  dst_align     Alignment of destination dataspace
 * \param  flags         Flags:
 *                       - #L4DM_CONTIGUOUS        create copy on phys. 
 *                                                 contiguos memory
 *                       - #L4DM_MEMPHYS_SAME_POOL use same memory pool like
 *                                                 source to allocate 
 *                                                 destination dataspace
 * \param  name          Destination dataspace name
 * \retval copy          Copy dataspace id
 *	
 * \return 0 on success (\a copy contains the id of the created copy),
 *         error code otherwise:
 *         - -#L4_EIPC       IPC error calling dataspace manager
 *         - -#L4_EINVAL     Invalid source dataspace id
 *         - -#L4_EPERM      Permission denied
 *         - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *         - -#L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
int
l4dm_memphys_copy(const l4dm_dataspace_t * ds, l4_offs_t src_offs, 
                  l4_offs_t dst_offs, l4_size_t num, int dst_pool, 
		  l4_addr_t dst_addr, l4_size_t dst_size, 
                  l4_addr_t dst_align, l4_uint32_t flags, const char * name, 
		  l4dm_dataspace_t * copy)
{
  l4_threadid_t dsm_id;
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    return -L4_ENODM;

  if ((ds == NULL) || !l4_thread_equal(dsm_id,ds->manager))
    return -L4_EINVAL;

  /* call DMphys */
  ret = if_l4dm_memphys_dmphys_copy_call(&dsm_id, ds->id, src_offs, 
                                         dst_offs, num, dst_pool, dst_addr, 
                                         dst_size, dst_align, flags, name,
                                         copy, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_phys: copy dataspace %u at DMphys ("l4util_idfmt") failed "
	    "(ret %d, exc %d)", ds->id, l4util_idstr(dsm_id), ret, _env.major);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }
  
  /* done */
  return ret;
}
