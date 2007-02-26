/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/open.c
 * \brief  DMphys client library, create new dataspace
 *
 * \date   01/09/2002
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
 * \brief  Create new dataspace (extended version for DMphys)
 * 
 * \param  pool          Memory pool
 * \param  addr          Start address of memory area
 *                         set to #L4DM_MEMPHYS_ANY_ADDR to find a suitable 
 *                         memory area
 * \param  size          Dataspace size
 * \param  align         Memory area alignment 
 * \param  flags         Flags:
 *                       - #L4DM_CONTIGUOUS  allocate contiguous memory area
 * \param  name          Dataspace name
 * \retval ds            Dataspace id
 *	
 * \return 0 on success (created dataspace, ds contains a valid dataspace id),
 *         error code otherwise:
 *         - -#L4_ENODM   DMphys not found
 *         - -#L4_EIPC    IPC error calling dataspace manager
 *         - -#L4_ENOMEM  out of memory
 *
 * Call DMphys to create a new dataspace.
 */
/*****************************************************************************/ 
int
l4dm_memphys_open(int pool, l4_addr_t addr, l4_size_t size, l4_addr_t align, 
		  l4_uint32_t flags, const char * name, l4dm_dataspace_t * ds)
{
  l4_threadid_t dsm_id;
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    return -L4_ENODM;
  
  LOGdL(DEBUG_OPEN, "DMphys at "l4util_idfmt, l4util_idstr(dsm_id));

  /* call DMphys */
  if (name != NULL)
    ret = if_l4dm_memphys_dmphys_open_call(&dsm_id, pool, addr, size, align,
                                           flags, name, ds, &_env);
  else
    ret = if_l4dm_memphys_dmphys_open_call(&dsm_id, pool, addr, size, align,
                                           flags, "", ds, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_phys: open dataspace at DMphys ("l4util_idfmt") failed "
            "(ret %d, exc %d)", l4util_idstr(dsm_id), ret, _env.major);
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }
  
  /* done */
  return ret;
}

