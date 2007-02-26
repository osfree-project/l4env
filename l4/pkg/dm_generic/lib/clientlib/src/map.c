/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/map.c
 * \brief  Generic dataspace manager client library, map implementation
 *
 * \date   01/08/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/bitops.h>
#include <l4/l4rm/l4rm.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Map dataspace region
 * 
 * \param  ds            Dataspace descriptor
 * \param  offs          Offset in dataspace
 * \param  size          Region size
 * \param  rcv_addr      Receive window address
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 * \param  flags	 Map flags
 * \retval fpage_addr    Map address of receive flexpage 
 * \retval fpage_size    Size of received flexpage
 *
 * \return 0 on success (mapped page), error code otherwise:
 *         - \c -L4_EIPC    IPC error calling dataspace manager
 *
 * Preconditions:
 * - correct receive window specification
 */
/*****************************************************************************/ 
static int
__do_map(l4dm_dataspace_t * ds, 
	 l4_offs_t offs, 
	 l4_size_t size, 
	 l4_addr_t rcv_addr, 
	 int rcv_size2, 
	 l4_offs_t rcv_offs, 
	 l4_uint32_t flags, 
	 l4_addr_t * fpage_addr, 
	 l4_size_t * fpage_size)
{
  int ret;
  l4_snd_fpage_t page;
  sm_exc_t _exc;

#if DEBUG_DO_MAP
  INFO("ds %u at %x.%x, offset 0x%08x\n",ds->id,
       ds->manager.id.task,ds->manager.id.lthread,offs);
  DMSG("  receive window at 0x%08x, size2 %d, offset 0x%08x\n",
       rcv_addr,rcv_size2,rcv_offs);
#endif

  /* do map call */
  ret = if_l4dm_generic_map(ds->manager,l4_fpage(rcv_addr,rcv_size2,0,0),
			    ds->id,offs,size,rcv_size2,rcv_offs,flags,
			    &page,&_exc);
  if ((ret < 0) || (_exc._type != exc_l4_no_exception))
    {
      if (ret < 0)
	return ret;
      else
	return -L4_EIPC;      
    }

  /* analyze received fpage */
#if DEBUG_DO_MAP
  INFO("got fpage at 0x%08x, size2 %d\n",
       page.fpage.fp.page << L4_LOG2_PAGESIZE,page.fpage.fp.size);
  DMSG("  snd_base 0x%08x, mapped to 0x%08x\n",page.snd_base,
       rcv_addr + page.snd_base);
#endif
       
  *fpage_addr = rcv_addr + page.snd_base;
  *fpage_size = 1UL << page.fpage.fp.size;

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
    return bsr((start - 1) ^ end);
  else
    {
      if (start == 0)
	return L4_WHOLE_ADDRESS_SPACE;
      else
	return bsf(start);
    }
}

/*****************************************************************************
 *** libdm_generic API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Map dataspace region (IDL wrapper)
 * 
 * \param  ds            Dataspace descriptor
 * \param  offs          Offset in dataspace
 * \param  size          Region size
 * \param  rcv_addr      Receive window address
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 * \param  flags         Flags:
 *                       - \c L4DM_RO          map read-only
 *                       - \c L4DM_RW          map read/write
 *                       - \c L4DM_MAP_PARTIAL allow partial mappings 
 *                       - \c L4DM_MAP_MORE    if possible, map more than the 
 *                                             specified dataspace region
 * \retval fpage_addr    Map address of receive fpage
 * \retval fpage_size    Size of receive fpage
 *
 * \return 0 on success (got fpage), error code otherwise:
 *         - \c -L4_EIPC         IPC error calling dataspace manager
 *         - \c -L4_EINVAL       invalid dataspace id or map / receive window 
 *                               size
 *         - \c -L4_EINVAL_OFFS  invalid dataspace / receive window offset
 *         - \c -L4_EPERM        permission denied
 *
 * For a detailed description of \c L4DM_MAP_PARTIAL and \c L4DM_MAP_MORE
 * see l4dm_map_pages().
 */
/*****************************************************************************/ 
int
l4dm_map_pages(l4dm_dataspace_t * ds, 
	       l4_offs_t offs, 
	       l4_size_t size, 
	       l4_addr_t rcv_addr, 
	       int rcv_size2, 
	       l4_offs_t rcv_offs, 
	       l4_uint32_t flags, 
	       l4_addr_t * fpage_addr, 
	       l4_size_t * fpage_size)
{
  int ret;
  
  /* do map */
  ret = __do_map(ds,offs,size,rcv_addr,rcv_size2,rcv_offs,flags,
		 fpage_addr,fpage_size);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      DMSG("ds %u at %x.%x, offset 0x%08x\n",ds->id,
	   ds->manager.id.task,ds->manager.id.lthread,offs);
      DMSG("receive window at 0x%08x, size2 %d, offset 0x%08x\n",
	   rcv_addr,rcv_size2,rcv_offs);
      ERROR("libdm_generic: map page failed: %d!",ret);
#endif
    }

  return ret;
}

/*****************************************************************************/
/**
 * \brief Map VM area
 * 
 * \param  ptr           VM address
 * \param  size          Area size
 * \param  flags         Flags:
 *                       - \c L4DM_RO          map read-only
 *                       - \c L4DM_RW          map read/write
 *                       - \c L4DM_MAP_PARTIAL allow partial mappings 
 *                       - \c L4DM_MAP_MORE    if possible, map more than the 
 *                                             specified VM region
 *	
 * \return 0 on success (mapped VM area), error code otherwise:
 *         - \c -L4_EIPC         IPC error calling regione mapper / 
 *                               dataspace manager
 *         - \c -L4_ENOTFOUND    No dataspace attached to parts of the VM area
 *         - \c -L4_EPERM        Permission denied
 *
 * Map the specified VM area. This will lookup the dataspaces which are
 * attached to the VM area and will call the dataspace managers to map the
 * dataspace pages. 
 * 
 * Flags:
 * - \c L4DM_MAP_PARTIAL allow partial mappings of the VM area. If no
 *                       dataspace is attached to a part of the VM area, 
 *                       just stop mapping and return without an error.
 * - \c L4DM_MAP_MORE    if possible, map more than the specified VM region. 
 *                       This allows l4dm_map to map more pages than specified
 *                       by \a ptr and \a size if a dataspace is attached to a 
 *                       larger VM region.
 */
/*****************************************************************************/ 
int
l4dm_map(void * ptr, 
	 l4_size_t size, 
	 l4_uint32_t flags)
{
  l4_addr_t addr;
  l4dm_dataspace_t ds;
  l4_offs_t offs,rcv_offs;
  l4_addr_t ds_map_addr,ds_map_end,fpage_addr;
  l4_size_t ds_map_size,fpage_size,map_size,size_mapped;
  l4_addr_t rcv_addr,rcv_end;
  int done,ret,rcv_size2;
  int align_start,align_end,align_size,align_addr;

  /* round addr / size to pagesize */
  addr = (l4_addr_t)ptr & L4_PAGEMASK;
  size = ((((l4_addr_t)ptr + size) + L4_PAGESIZE - 1) & L4_PAGEMASK) - addr;

#if DEBUG_MAP
  INFO("map VM area 0x%08x - 0x%08x\n",addr,addr + size);
#endif

  /* map, repeat until the requested area is mapped completely */
  done = 0;
  while (!done)
    {
      /* lookup address */
      ret = l4rm_lookup((void *)addr,&ds,&offs,&ds_map_addr,&ds_map_size);
      if (ret < 0)
	{
	  if (ret == -L4_ENOTFOUND)
	    {
	      /* no dataspace attached to the VM addr */
	      if (flags & L4DM_MAP_PARTIAL)
		return 0;
	    }

	  ERROR("libdm_generic: lookup address 0x%08x failed: %d!",addr,ret);
	  return ret;
	}
      ds_map_end = ds_map_addr + ds_map_size;

#if DEBUG_MAP
      INFO("addr 0x%08x\n",addr);
      DMSG("  ds %u at %x.%x, offs 0x%08x, map area 0x%08x-0x%08x\n",
	   ds.id,ds.manager.id.task,ds.manager.id.lthread,offs,
	   ds_map_addr,ds_map_end);
#endif

      /* calculate receive window */
      if (flags & L4DM_MAP_MORE)
	{
	  /* find the max. receive window in the map area which includes ptr.
	   * align_start is the max. alignment of an address in the address 
	   * range (ds_map_addr,addr), which is the max. size of an receive 
	   * window with a start address in that range, align_end is the 
	   * max. alignment of an address in the address range
	   * (addr + L4_PAGESIZE,ds_map_end), which is the max. size of 
	   * a receive window with an end address in that area. The max.
	   * receive window size is the minimum of align_start and align_end 
	   */
	  align_start = __max_addr_align(ds_map_addr,addr);
	  align_end = __max_addr_align(addr + L4_PAGESIZE,ds_map_end);
	  rcv_size2 = (align_start < align_end) ? align_start : align_end;
#if DEBUG_MAP
	  DMSG("  align start %d, align end %d => receive window size %d\n",
	       align_start,align_end,rcv_size2);
#endif
	}
      else
	{
	  /* calculate the max. receive window size with the start address at 
	   * ptr in the dataspace map area. */
	  align_size = bsr(ds_map_end - addr);
	  align_addr = bsf(addr);
	  rcv_size2 = (align_addr < align_size) ? align_addr : align_size;
#if DEBUG_MAP
	  DMSG("  align addr %d, align size %d => receive window size %d\n",
	       align_addr,align_size,rcv_size2);
#endif
	}
      rcv_addr = addr & ~((1UL << rcv_size2) - 1);
      rcv_end = rcv_addr + (1UL << rcv_size2);
      rcv_offs = addr - rcv_addr;

      map_size = rcv_end - addr;
      if (map_size > size)
	map_size = size;

#if DEBUG_MAP
      DMSG("  receive window at 0x%08x-0x%08x, size %d\n",
	   rcv_addr,rcv_end,rcv_size2);
      DMSG("  rcv offs 0x%08x, map size 0x%08x\n",rcv_offs,map_size);
#endif
      
      /* map, allow partial mapping */
      ret = __do_map(&ds,offs,size,rcv_addr,rcv_size2,rcv_offs,
		     flags | L4DM_MAP_PARTIAL,&fpage_addr,&fpage_size);
      if (ret < 0)
	{
	  /* map failed */
	  if (ret == -L4_EINVAL_OFFS)
	    {
	      /* invalid offset in dataspace, this can happen if the 
	       * dataspace which is attached to the VM region is smaller 
	       * than the size of the VM region */
	      if (flags & L4DM_MAP_PARTIAL)
		return 0;
	    }

#if DEBUG_ERRORS
	  DMSG("l4dm_map: addr 0x%08x, ds %d at %x.%x, offs 0x%08x\n",
	       addr,ds.id,ds.manager.id.task,ds.manager.id.lthread,offs);
	  DMSG("  dataspace map area 0x%08x-0x%08x\n",ds_map_addr,ds_map_end);
	  DMSG("  receive window at 0x%08x, size2 %d, rcv offs 0x%08x\n",
	       rcv_addr,rcv_size2,rcv_offs);
	  ERROR("libdm_generic: map failed: %d!",ret);
#endif
	  return ret;
	}
      size_mapped = fpage_size - (addr - fpage_addr);

#if DEBUG_MAP
      INFO("got fpage\n");
      DMSG("  fpage at 0x%08x, size 0x%08x, size mapped 0x%08x\n",
	   fpage_addr,fpage_size,size_mapped);
#endif

      if (size_mapped >= size)
	done = 1;
      else
	{
	  /* requested area not yet fully mapped */
	  addr += size_mapped;
	  size -= size_mapped;
	}
    }

  /* done */
  return 0;
}
