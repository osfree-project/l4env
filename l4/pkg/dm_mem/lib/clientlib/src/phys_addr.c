/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/phys_addr.c
 * \brief  Memory dataspace manager client library, get phys. address
 *
 * \date   01/29/2002
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
#include <l4/l4rm/l4rm.h>

/* DMmem includes */
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_mem/dm_mem-client.h>
#include "__debug.h"
#include "__phys_addr.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to request the phys. address of a dataspace 
 *         region
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 * \retval paddr         Phys. address
 * \retval psize         Size of phys. contiguous dataspace region at offset
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC         IPC error calling dataspace manager
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
__get_phys_addr(const l4dm_dataspace_t * ds, l4_offs_t offset, l4_size_t size,
		l4_addr_t * paddr, l4_size_t * psize)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_mem_phys_addr_call(&(ds->manager), ds->id, offset, size, 
                                   paddr, psize, &_env);
  if (ret || DICE_HAS_EXCEPTION(&_env))
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_mem: get address of dataspace %u at "l4util_idfmt \
            " failed (ret %d, exc %d)!", ds->id, l4util_idstr(ds->manager), 
	    ret, DICE_EXCEPTION_MAJOR(&_env));
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;
}
