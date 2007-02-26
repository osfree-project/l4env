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
 *** libdm_mem API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Get phys. address of a of VM region
 * 
 * \param  ptr           VM region address
 * \param  size          VM region size
 * \param  addrs         Phys. address regions destination buffer
 * \param  num           Number of elements in \a addrs.
 * \retval psize         Total size of the address regions in \a addrs
 *	
 * \return On success number of found phys. address regions, 
 *         error code otherwise:
 *         - -#L4_ENOTFOUND  dataspace not found for a VM address
 *         - -#L4_EINVAL     Invalid vm region (i.e. no dataspace attached to 
 *                           that region, but external pager etc.)
 *         - -#L4_EPERM      caller is not a client of the dataspace
 *         - -#L4_EIPC       error calling region mapper / dataspace manager
 *         
 * Lookup the phys. addresses for the specified virtual memory region. 
 */
/*****************************************************************************/ 
int
l4dm_mem_phys_addr(const void * ptr, l4_size_t size, l4dm_mem_addr_t addrs[], 
                   int num, l4_size_t * psize)
{
  int ret,i;
  l4_addr_t addr = (l4_addr_t)ptr; 
  l4dm_dataspace_t ds;
  l4_offs_t ds_offs;
  l4_addr_t ds_map_addr;
  l4_size_t left,ds_map_size,map_remain;
  l4_threadid_t dummy;

  if (size <= 0)
    /* nothing to do */
    return 0;

  /* lookup phys. addresses for VM region */
  left = size;
  i = 0;
  while ((left > 0) && (i < num))
    {
      LOGdL(DEBUG_PHYS_ADDR, "addr 0x"l4_addr_fmt", left 0x"l4_addr_fmt"",
	  addr, (l4_addr_t)left);

      /* lookup addr */
      ret = l4rm_lookup((void *)addr, &ds_map_addr, &ds_map_size, 
                        &ds, &ds_offs, &dummy);
      if (ret < 0)
	{
	  LOGdL(DEBUG_ERRORS, "libdm_mem: lookup VM addr 0x"l4_addr_fmt
	      " failed: %d!", addr, ret);
	  return ret;
	}

      if (ret != L4RM_REGION_DATASPACE)
        {
          LOGdL(DEBUG_ERRORS, "trying to get phys. address of non-dataspace " \
                "region at addr 0x"l4_addr_fmt" (type %d)", addr, ret);
          return -L4_EINVAL;
        }

      map_remain = ds_map_size - ds_offs;
      if (map_remain > left)
	map_remain = left;
       
      LOGdL(DEBUG_PHYS_ADDR, "ds %u at "l4util_idfmt"\n" \
            " ds map area 0x"l4_addr_fmt"-0x"l4_addr_fmt", offs 0x"l4_addr_fmt
	    "\n request region size 0x"l4_addr_fmt,
            ds.id, l4util_idstr(ds.manager), ds_map_addr,
            ds_map_addr + ds_map_size, ds_offs, (l4_addr_t)map_remain);

      /* get phys. addr */
      ret = __get_phys_addr(&ds, ds_offs, map_remain,
			    &addrs[i].addr, &addrs[i].size);
      if (ret < 0)
	{
#if DEBUG_ERRORS
	  LOG_printf("ds %u at "l4util_idfmt", offset 0x"l4_addr_fmt
	         ", size 0x"l4_addr_fmt"\n", ds.id, l4util_idstr(ds.manager),
		 ds_offs, (l4_addr_t)map_remain);
	  LOGL("get phys. address failed: %d!", ret);
#endif
	  if ((ret == -L4_EINVAL) || (ret == -L4_EINVAL_OFFS))
	    /* invalid offset, this can result from weird VM layouts */
	    return -L4_ENOTFOUND;
	  else
	    return ret;
	}
      
      LOGdL(DEBUG_PHYS_ADDR, "addr index %d: addr 0x"l4_addr_fmt
	    ", size 0x"l4_addr_fmt, i, addrs[i].addr, (l4_addr_t)addrs[i].size);

      left -= addrs[i].size;
      addr += addrs[i].size;
      i++;
    }

  /* set phys. region size */
  *psize = size - left;

  /* done */
  return i;
}
