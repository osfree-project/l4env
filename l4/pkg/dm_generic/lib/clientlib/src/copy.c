/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/copy.c
 * \brief  Generic dataspace manager client library, copy dataspace
 *
 * \date   01/28/2002
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

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>
#include "__debug.h"

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
 *         - -#L4_EIPC       IPC error calling dataspace manager
 *         - -#L4_EINVAL     Invalid source dataspace id
 *         - -#L4_EPERM      Permission denied
 *         - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *         - -#L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
static int
__do_copy(const l4dm_dataspace_t * ds, l4_offs_t src_offs, l4_offs_t dst_offs,
	  l4_size_t num, l4_uint32_t flags, const char * name, 
	  l4dm_dataspace_t * copy)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;
  
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  if (name != NULL)
    ret = if_l4dm_generic_copy_call(&(ds->manager), ds->id, src_offs, dst_offs,
                                    num, flags, name, copy, &_env);
  else
    ret = if_l4dm_generic_copy_call(&(ds->manager), ds->id, src_offs, dst_offs,
                                    num, flags, "", copy, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      LOGdL(DEBUG_ERRORS, "libdm_generic: copy ds %u at "l4util_idfmt \
            " failed (ret %d, exc %d)", ds->id, l4util_idstr(ds->manager),
            ret, _env.major);
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
 *                       - #L4DM_COW         create copy-on-write copy
 *                       - #L4DM_PINNED      create copy on pinned memory
 *                       - #L4DM_CONTIGUOUS  create copy on phys. contiguos 
 *                                             memory
 * \param  name          Copy name
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
l4dm_copy(const l4dm_dataspace_t * ds, l4_uint32_t flags, const char * name, 
	  l4dm_dataspace_t * copy)
{
  /* create copy */
  return __do_copy(ds, 0, 0, L4DM_WHOLE_DS, flags, name, copy);
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
 *                       - #L4DM_COW         create copy-on-write copy
 *                       - #L4DM_PINNED      create copy on pinned memory
 *                       - #L4DM_CONTIGUOUS  create copy on phys. contiguos 
 *                                           memory
 * \param  name          Copy name
 * \retval copy          Dataspace id of copy
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC       IPC error calling dataspace manager
 *         - -#L4_EINVAL     Invalid source dataspace id
 *         - -#L4_EPERM      Permission denied
 *         - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *         - -#L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
int
l4dm_copy_long(const l4dm_dataspace_t * ds, l4_offs_t src_offs, 
               l4_offs_t dst_offs, l4_size_t num, l4_uint32_t flags, 
               const char * name, l4dm_dataspace_t * copy)
{
  /* create copy */
  return __do_copy(ds, src_offs, dst_offs, num, flags, name, copy);
}
