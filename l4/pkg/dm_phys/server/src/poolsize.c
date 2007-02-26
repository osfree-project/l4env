/*****************************************************************************/
/**
 * \file   dm_phys/server/src/poolsize.c
 * \brief  DMPhys, get the size of a pool
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
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

/* DMphys/private include */
#include "dm_phys-server.h"
#include "__debug.h"
#include "__pages.h"

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return size of memory pool
 * 
 * \param  _dice_corba_obj    Request source
 * \param  pool               Memory pool number
 * \param  _dice_corba_env    Server environment
 * \retval size               Size of memory pool
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid memory pool
 */
/*****************************************************************************/ 
l4_int32_t
if_l4dm_memphys_dmphys_poolsize_component(CORBA_Object _dice_corba_obj,
                                          l4_uint32_t pool,
                                          l4_uint32_t * size,
                                          l4_uint32_t * free,
                                          CORBA_Server_Environment * _dice_corba_env)
{
  page_pool_t * p = dmphys_get_page_pool(pool);
  
  /* sanity checks */
  if (p == NULL)
    {
      LOGdL(DEBUG_ERRORS, "DMphys: invalid page pool (%d)", pool);
      return -L4_EINVAL;
    }

  /* return size of memory pool, respect reserved memory since this portion 
   * is not used to allocate client dataspaces */
  *size = p->size - p->reserved;
  *free = p->free;

  return 0;
}

