/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/poolsize.c
 * \brief  DMPhys client library, get pool size
 *
 * \date   05/02/2004
 * \author Ronald Aigner <ra3@os.inf.tu-dresden.de
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
#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_phys/dm_phys-client.h>
#include "__debug.h"

/*****************************************************************************/
/**
 * \brief  Return size of memory pool
 *   
 * \param  pool          Pool number
 * \retval size          Memory pool size
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  Invalid memory pool
 *         - -#L4_ENODM   DMphys not found
 *         - -#L4_EIPC    Dataspace manager call failed
 */
/*****************************************************************************/ 
int
l4dm_memphys_poolsize(int pool, l4_size_t * size, l4_size_t * free)
{
  l4_threadid_t dsm_id;
  int ret;
  CORBA_Environment _env = dice_default_environment;
  
  /* get DMphys thread id */
  dsm_id = l4dm_memphys_find_dmphys();
  if (l4_is_invalid_id(dsm_id))
    return -L4_ENODM;
  
  /* call DMphys */
  ret = if_l4dm_memphys_dmphys_poolsize_call(&dsm_id, pool, size, free, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS,
	  "libdm_phys: poolsize call at DMphys ("l4util_idfmt") failed "
	  "(ret %d, exc %d)", l4util_idstr(dsm_id), ret, _env.major);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }
  
  /* done */
  return ret;
}

