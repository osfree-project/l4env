/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/lib/src/pagesize.c
 * \brief  DMphys client library, check pagesize of memory area
 *
 * \date   02/09/2002
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
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/l4rm/l4rm.h>

/* DMphys includes */
#include <l4/dm_phys/dm_phys.h>
#include <l4/dm_phys/dm_phys-client.h>
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to check pagesize
 * 
 * \param  ds            Dataspace id
 * \param  offs          Offset in dataspace
 * \param  size          Dataspace region size
 * \param  pagesize      Pagesize
 * \retval ok            1 if dataspace region can be mapped with given
 *                       pagesize, 0 if not
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC        IPC error calling DMphys
 *         - \c -L4_EPERM       operation not permitted
 *         - \c -L4_EINVAL      invalid dataspace id
 *         - \c -L4_EINVAL_OFFS offset points beyond end of dataspace
 */
/*****************************************************************************/ 
static int
__check_pagesize(l4dm_dataspace_t * ds, 
		 l4_offs_t offs, 
		 l4_size_t size,
		 int pagesize, 
		 int * ok)
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

  /* call dataspace manager */
  ret = if_l4dm_memphys_dmphys_pagesize_call(&dsm_id,
      ds->id, offs, size, pagesize, (l4_uint32_t*)ok, &_env);
  if (ret || (_env.major != CORBA_NO_EXCEPTION))
    {
      ERROR("libdm_phys: check pagesize at DMphys (%x.%x) failed "
	    "(ret %d, exc %d)",dsm_id.id.task,dsm_id.id.lthread,
	    ret,_env.major);
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
 * \brief Calculate the max. alignment of an address in address range
 * 
 * \param  start         Address range start address
 * \param  end           Address range end address
 *	
 * \return Alignment (log2)
 *
 * Find alignment of the address with the max. alignment in the given address 
 * range. It is calculated from:
 *
 *   (start != end) ? bsr((start - 1) xor end) : bsf(start)
 *
 * where:
 * - bsf(start) (bit scan forward) finds the least significant bit set 
 *   => it's alignment of the start address
 * - bsr((start - 1) xor end) (bit scan reverse) finds the most significant 
 *   bit set => find the most significant bit which is different in 
 *   (start - 1) and end. The bit number is the max. alignment of an address
 *   in that address range. 
 * 
 * Preconditions:
 * - start/end must be page aligned
 */
/*****************************************************************************/ 
static inline int
__max_addr_align(l4_addr_t start, 
		 l4_addr_t end)
{
  if (start != end)
    return l4util_bsr((start - 1) ^ end);
  else
    {
      if (start == 0)
	return L4_WHOLE_ADDRESS_SPACE;
      else
	return l4util_bsf(start);
    }
}

/*****************************************************************************
 *** DMphys client lib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Check pagesize of dataspace region, IDL wrapper
 * 
 * \param  ds            Dataspace id
 * \param  offs          Offset in dataspace
 * \param  size          Dataspace region size
 * \param  pagesize      Log2 Pagesize
 * \retval ok            1 if dataspace region can be mapped with given
 *                       pagesize, 0 if not
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EIPC        IPC error calling DMphys
 *         - \c -L4_EPERM       operation not permitted
 *         - \c -L4_EINVAL      invalid dataspace id
 *         - \c -L4_EINVAL_OFFS offset points beyond end of dataspace
 */
/*****************************************************************************/ 
int
l4dm_memphys_pagesize(l4dm_dataspace_t * ds, 
		      l4_offs_t offs, 
		      l4_size_t size, 
		      int pagesize, 
		      int * ok)
{
  /* check pagesize */
  return __check_pagesize(ds,offs,size,pagesize,ok);
}

/*****************************************************************************/
/**
 * \brief  Check if dataspaces attached to VM area can be mapped
 *         with a certain pagesize.
 * 
 * \param  ptr           VM area address
 * \param  size          VM area size
 * \param  pagesize      Log2 pagesize
 *	
 * \return 1 if dataspace can be mapped with given pagesize, 0 if not
 */
/*****************************************************************************/ 
int
l4dm_memphys_check_pagesize(void * ptr, 
			    l4_size_t size, 
			    int pagesize)
{
  l4_addr_t addr = (l4_addr_t)ptr;
  l4_addr_t end;
  int ret;
  l4dm_dataspace_t ds;
  l4_addr_t ds_map_addr,ds_map_end,page_addr;
  l4_offs_t ds_map_offs,page_offs;
  l4_size_t ds_map_size,page_size,ps;
  int align_start,align_end,ok;
  
  LOGdL(DEBUG_PAGESIZE,"\n  area 0x%08x-0x%08x, pagesize %d",
        addr,addr + size,pagesize);
  
  /* on ia32, only 4KB pages (pagesize 12) and 4MB pages (pagesize 22) are
   * supproted */
  if ((pagesize != L4_LOG2_PAGESIZE) && (pagesize != L4_LOG2_SUPERPAGESIZE))
    {
      Error("libdm_phys: invalid pagesize %d!",pagesize);
      return 0;
    }

  end = addr + size;
  while (addr < end)
    {
#if DEBUG_PAGESIZE
      printf("  addr 0x%08x\n",addr);
#endif

      /* lookup dataspace attached to address */
      ret = l4rm_lookup((void *)addr,&ds,&ds_map_offs,&ds_map_addr,
			&ds_map_size);
      if (ret < 0)
	{
	  ERROR("libdm_phys: lookup address 0x%08x failed: %d!",addr,ret);
	  return 0;
	}
      ds_map_end = ds_map_addr + ds_map_size;

#if DEBUG_PAGESIZE
      printf("  dataspace %u at %x.%x\n",ds.id,ds.manager.id.task,
             ds.manager.id.lthread);
      printf("  attached to 0x%08x-0x%08x, offs 0x%08x\n",ds_map_addr,
             ds_map_end,ds_map_offs);
#endif

      /* first, check if the alignment of the attached region allows a 
       * receive window with the requested pagesize, see map.c for further 
       * explanations */
      align_start = __max_addr_align(ds_map_addr,addr);
      align_end = __max_addr_align(addr + L4_PAGESIZE,ds_map_end);
      
#if DEBUG_PAGESIZE
      printf("  align start %d, align end %d\n",align_start,align_end);
#endif

      if ((align_start < pagesize) || (align_end < pagesize))
	/* receive window to small */
	return 0;

      /* receive window large enough, call dataspace manager to check if the
       * dataspace region can be provided with the pagesize */
      page_addr = addr & ~((1UL << pagesize) - 1);
      page_offs = ds_map_offs - (addr - page_addr);
      ps = 1UL << pagesize;
      page_size = ps;

      /* make check area as large as possible */
      while (((page_addr + page_size + ps) <= ds_map_end) &&
	     ((addr + ((page_addr + page_size + ps) - addr)) <= end))
	page_size += ps;

#if DEBUG_PAGESIZE
      printf("  page at 0x%08x, offs 0x%08x, size 0x%08x\n",
             page_addr,page_offs,page_size);
#endif
      
      ret = __check_pagesize(&ds,page_offs,page_size,pagesize,&ok);
      if ((ret < 0) || !ok)
	{
#if DEBUG_PAGESIZE
	  printf("  ok = %d, ret = %d\n",ok,ret);
#endif	    
	  /* check failed */
	  return 0;
	}

      addr += ((page_addr + page_size) - addr);
    }
  
  /* test succeeded */
  return 1;
}
