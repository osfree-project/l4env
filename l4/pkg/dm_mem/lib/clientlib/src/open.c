/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/open.c
 * \brief  Memory dataspace manager client library, open new dataspace
 *
 * \date   11/23/2001
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
#include <l4/env/env.h>
#include <l4/util/macros.h>

/* DMmem includes */
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_mem/dm_mem-client.h>
#include "__debug.h"

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Create new dataspace
 * 
 * \param  dsm_id        Dataspace manager id
 * \param  size          Dataspace size
 * \param  align         Alignment
 * \param  flags         Flags
 * \param  name          Dataspace name
 * \retval ds            Dataspace id
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC    IPC error calling dataspace manager
 *         - -#L4_ENOMEM  out of memory
 *         - -#L4_ENODM   no dataspace manager found
 *
 * Call dataspace manager \a dsm_id to create a new dataspace.
 */
/*****************************************************************************/ 
int
l4dm_mem_open(l4_threadid_t dsm_id, l4_size_t size, l4_addr_t align, 
	      l4_uint32_t flags, const char * name, 
	      l4dm_dataspace_t * ds)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  if (l4_thread_equal(dsm_id, L4DM_DEFAULT_DSM))
    {
      /* request dataspace manager id from L4 environment */
      dsm_id = l4env_get_default_dsm();
      if (l4_is_invalid_id(dsm_id))
	{
	  LOGdL(DEBUG_ERRORS, "libdm_mem: no dataspace manager found!");
	  return -L4_ENODM;
	}
    }

  /* call dataspace manager */
  if (name != NULL)
    ret = if_l4dm_mem_open_call(&dsm_id, size, align, flags, name, ds, &_env);
  else
    ret = if_l4dm_mem_open_call(&dsm_id, size, align, flags, "", ds, &_env);
  if (ret || DICE_HAS_EXCEPTION(&_env))
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_mem: open dataspace at "l4util_idfmt" failed " \
            "(ret %d, exc %d)", l4util_idstr(dsm_id), ret,
	    DICE_EXCEPTION_MAJOR(&_env));
      if (ret)
        return ret;
      else
	return -L4_EIPC;
    }

  /* done */
  return ret;
}

