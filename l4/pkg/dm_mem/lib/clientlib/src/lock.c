/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_mem/client-lib/src/lock.c
 * \brief  Memory dataspace manager client library, lock/unlock dataspaces
 *
 * \date   01/30/2002
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

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to lock dataspace region
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC         IPC error calling dataspace manager
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
static int
__lock(const l4dm_dataspace_t * ds, l4_offs_t offset, l4_size_t size)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_mem_lock_call(&(ds->manager), ds->id, offset, size, &_env);
  if (ret || DICE_HAS_EXCEPTION(&_env))
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_mem: lock dataspace %u at "l4util_idfmt" failed " \
            "(ret %d, exc %d)!", ds->id, l4util_idstr(ds->manager), 
            ret, DICE_EXCEPTION_MAJOR(&_env));
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;  
}

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to unlock dataspace region
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC         IPC error calling dataspace manager
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
static int
__unlock(const l4dm_dataspace_t * ds, l4_offs_t offset, l4_size_t size)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  /* call dataspace manager */
  ret = if_l4dm_mem_unlock_call(&(ds->manager), ds->id, offset, size, &_env);
  if (ret || DICE_HAS_EXCEPTION(&_env))
    {
      LOGdL(DEBUG_ERRORS, 
            "libdm_mem: unlock dataspace %u at "l4util_idfmt" failed " \
            "(ret %d, exc %d)!", ds->id, l4util_idstr(ds->manager), 
            ret, DICE_EXCEPTION_MAJOR(&_env));
      if (ret)
        return ret;
      else
	return -L4_EIPC;      
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Walk through VM region to lock/unlock attached dataspaces
 * 
 * \param  addr          VM region start address
 * \param  size          VM region size
 * \param  lock          1 .. lock, 0 .. unlock atached dataspaces
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_ENOTFOUND  dataspace not found for a VM address
 *         - -#L4_EINVAL     Invalid vm region (i.e. no dataspace attached to 
 *                           that region, but external pager etc.)
 *         - -#L4_EIPC       error calling region mapper / dataspace manager
 *         - -#L4_EPERM      caller is not a client of a dataspace attached
 *                           to the VM region
 */
/*****************************************************************************/ 
static int
__walk_vm(l4_addr_t addr, l4_size_t size, int lock)
{
  int ret,failed;
  l4_addr_t a,ds_map_addr;
  l4_size_t left,ds_map_size,map_remain;
  l4_offs_t ds_offs;
  l4dm_dataspace_t ds;
  l4_threadid_t dummy;

  a = addr;
  left = size;
  failed = 0;
  while ((left > 0) && !failed)
    {
      LOGdL(DEBUG_LOCK, "addr 0x"l4_addr_fmt", left 0x"l4_addr_fmt"",
	    a, (l4_addr_t)left);
      
      /* lookup addr */
      ret = l4rm_lookup((void *)a, &ds_map_addr, &ds_map_size, 
                        &ds, &ds_offs, &dummy);
      if (ret < 0)
	{
	  LOGdL(DEBUG_ERRORS, "libdm_mem: lookup VM addr 0x"l4_addr_fmt
	        " failed: %d!", addr, ret);
	  failed = ret;
	}
      
      if (ret != L4RM_REGION_DATASPACE)
        {
          LOGdL(DEBUG_ERRORS, "trying to lock non-dataspace " \
                "region at addr 0x"l4_addr_fmt" (type %d)", a, ret);
          failed = -L4_EINVAL;
        }

      if (!failed)
	{
	  /* lock / unlock dataspace region */
	  map_remain = ds_map_size - ds_offs;
	  if (map_remain > left)
	    map_remain = left;

	  LOGdL(DEBUG_LOCK, "ds %u at "l4util_idfmt"\n" \
                " ds map area 0x"l4_addr_fmt"-0x"l4_addr_fmt", offs 0x"
		l4_addr_fmt"\n %s request size 0x"l4_addr_fmt,
                ds.id, l4util_idstr(ds.manager), 
                ds_map_addr, ds_map_addr + ds_map_size, ds_offs,
                (lock) ? "lock" : "unlock",(l4_addr_t)map_remain);

	  if (lock)
	    ret = __lock(&ds, ds_offs, map_remain);
	  else
	    ret = __unlock(&ds, ds_offs, map_remain);
	  if (ret < 0)
	    {
#if DEBUG_ERRORS
	      LOG_printf("ds %u at "l4util_idfmt
		          ", offset 0x"l4_addr_fmt", size 0x"l4_addr_fmt"\n",
                     ds.id, l4util_idstr(ds.manager), ds_offs,
		     (l4_addr_t)map_remain);
	      LOGL("libdm_mem: %s failed: %d!", (lock) ? "lock" : "unlock", 
                   ret);
#endif
	      if ((ret == -L4_EINVAL) || (ret == -L4_EINVAL_OFFS))
		/* invalid offset, this can result from weird VM layouts */
		failed = -L4_ENOTFOUND;
	      else
		failed = ret;
	    }
	  else
	    {
	      left -= map_remain;
	      a += map_remain;
	    }
	}
    }

  if (failed && lock)
    {
      /* unlock already locked dataspace regions */
      if ((size - left) > 0)
	__walk_vm(addr, size - left, 0);
    }

  /* done */
  return failed;
}

/*****************************************************************************
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Lock dataspace region, IDL call wrapper
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC         IPC error calling dataspace manager
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_lock(const l4dm_dataspace_t * ds, 
                 l4_offs_t offset, l4_size_t size)
{
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  return __lock(ds, offset, size);
}

/*****************************************************************************/
/**
 * \brief  Unlock dataspace region, IDL call wrapper
 * 
 * \param  ds            Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC         IPC error calling dataspace manager
 *         - -#L4_EPERM        caller is not a client of the dataspace
 *         - -#L4_EINVAL       invalid dataspace id
 *         - -#L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_mem_ds_unlock(const l4dm_dataspace_t * ds, 
                   l4_offs_t offset, l4_size_t size)
{
  if (ds == NULL)
    return -L4_EINVAL;

  /* call dataspace manager */
  return __unlock(ds, offset, size);
}

/*****************************************************************************/
/**
 * \brief  Lock virtual memory region
 * 
 * \param  ptr           VM region start address
 * \param  size          VM region size
 *	
 * \return 0 on success (locked VM region), error code otherwise:
 *         - -#L4_ENOTFOUND  dataspace not found for a VM address
 *         - -#L4_EINVAL     Invalid vm region (i.e. no dataspace attached to 
 *                           that region, but external pager etc.)
 *         - -#L4_EIPC       error calling region mapper / dataspace manager
 *         - -#L4_EPERM      caller is not a client of a dataspace attached
 *                           to the VM region
 */
/*****************************************************************************/ 
int
l4dm_mem_lock(const void * ptr, l4_size_t size)
{
  /* lock VM region */
  return __walk_vm((l4_addr_t)ptr, size, 1);
}

/*****************************************************************************/
/**
 * \brief  Unlock virtual memory region
 * 
 * \param  ptr           VM region start address
 * \param  size          VM region size
 *	
 * \return 0 on success (unlocked VM region), error code otherwise:
 *         - -#L4_ENOTFOUND  dataspace not found for a VM address
 *         - -#L4_EINVAL     Invalid vm region (i.e. no dataspace attached to 
 *                           that region, but external pager etc.)
 *         - -#L4_EIPC       error calling region mapper / dataspace manager
 *         - -#L4_EPERM      caller is not a client of a dataspace attached
 *                           to the VM region
 */
/*****************************************************************************/ 
int
l4dm_mem_unlock(const void * ptr, l4_size_t size)
{
  /* lock VM region */
  return __walk_vm((l4_addr_t)ptr, size, 0);
}
