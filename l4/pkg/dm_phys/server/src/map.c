/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/map.c
 * \brief  DMphys, page map/unmap
 *
 * \date   11/24/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include <l4/dm_phys/consts.h>
#include "__dm_phys.h"
#include "__dataspace.h"
#include "__pages.h"
#include "__memmap.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

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
	return DMPHYS_AS_LOG2_SIZE;
      else
	return l4util_bsf(start);
    }
}

/*****************************************************************************/
/**
 * \brief Calculate max. fpage size for page area / remote alignment
 * 
 * \param  area          Page area descriptor
 * \param  offset        Offset in page area
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 *	
 * \return Max. fpage size (log2)
 *	
 * Calculate the max. fpage size possible with the given page area and 
 * receive window. The fpage address can be any address of the page area 
 * (in contrast to __max_size_at_offs), but must contain the page at 
 * the given offset. The size is calculated from:
 * 
 *   min(bsf(local_addr xor rcv_offs),
 *       __max_addr_align(area_start_addr,local_addr),
 *       __max_addr_align((local_addr + PAGESIZE),area_end_addr))
 *
 * where
 * - (1) bsf(local_addr xor rcv_offs) is the max. alignment common to the
 *   local map address and the remote map offset (it's the least significant 
 *   bit which is different in local_addr and rcv_offs). 
 *   If local_addr == rcv_offs, the max size (and thus the alignment) is
 *   the size of the receive window
 * - (2) __max_addr_align(area_start_addr,local_addr) is the max. alignment of 
 *   an address in (area_start_addr, local_addr) => the max. size of a fpage 
 *   with a start address in (area_start_addr,local_addr).
 * - (3) __max_addr_align((local_addr + PAGESIZE),area_end_addr) is the max. 
 *   alignment of an address in ((local_addr + PAGESIZE), area_end_addr) 
 *   => the max. size of a fpage with an end address in 
 *   ((local_addr + PAGESIZE), area_end_addr).
 *   => together with (2) this defines the max. fpage which contains the page
 *      at the given offset in the page area
 * 
 * Preconditions:
 * - addresses / offsets page aligned
 */
/*****************************************************************************/ 
static inline int
__max_size(page_area_t * area, 
	   l4_offs_t offset, 
	   int rcv_size2,
	   l4_offs_t rcv_offs)
{
  l4_addr_t area_start = AREA_MAP_ADDR(area);
  l4_addr_t local_addr = area_start + offset;
  l4_addr_t area_end   = area_start + area->size;
  int align_addr,align_start,align_end,align;
#if MAP_FPAGE_PARANOIA
  l4_addr_t map_start,map_end;
  int align_size;
#endif

  /* alignment local/remote addr (1) */
  if (local_addr == rcv_offs)
    align_addr = rcv_size2;
  else
    {
      align_addr = l4util_bsf(local_addr ^ rcv_offs);
      if (align_addr > rcv_size2)
	align_addr = rcv_size2;
    }

  /* alignment fpage start (2) */
  align_start = __max_addr_align(area_start,local_addr);
  
  /* alignment fpage end (3) */
  align_end = __max_addr_align(local_addr,area_end);

  align = (align_start < align_end) ? align_start : align_end;
  if (align_addr < align)
    align = align_addr;

  LOGdL(DEBUG_MAP,"area 0x%08x-0x%08x\n" \
        "  addr 0x%08x\n" \
        "  remote offset 0x%08x, remote size %d\n" \
        "  align addr %d, align start %d, align end %d, align %d",
        area_start,area_end,local_addr,rcv_offs,rcv_size2,align_addr,
        align_start,align_end,align);

#if MAP_FPAGE_PARANOIA
  /* check if size alignment calculation (2)/(3) works */
  align_size = (align_start < align_end) ? align_start : align_end;
  align_size++;
  map_start = local_addr & ~((1UL << align_size) - 1);
  map_end = map_start + (1UL << align_size);
  if ((map_start >= area_start) && (map_end <= area_end))
    {
      printf("area 0x%08x-0x%08x, offset 0x%08x\n",area_start,area_end,offset);
      printf("map area 0x%08x-0x%08x (size %d)\n",map_start,map_end,align_size);
      Panic("DMphys: wrong size alignment: got %d, but %d also works!\n",
	    align_size - 1,align_size);
    } 
#endif

  /* done */
  return align;
}

/*****************************************************************************/
/**
 * \brief Calculate max. fpage size with given offset / alignment / page area
 * 
 * \param  area          Page area descriptor
 * \param  offset        Offset in page area
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 *	
 * \return Max. fpage size (log2)
 *
 * The max. fpage size is calculated from 
 *
 *   min(bsf(local_addr | rcv_offs),bsr(area_size - offset))
 * 
 * where
 * - bsf(local_addr | rcv_offs) is the max. alignment allowed by the 
 *   local/remote address (bsf finds the least significant bit set)
 *   If the receive window offset is 0, the remote alignment is the receive 
 *   window size
 * - bsr(area_size - offset) the max. size (log2) allowed by the remainder
 *   of the page area
 * 
 * Preconditions:
 * - offset/rcv_offs page aligned
 * - offset < area->size
 * - rcv_offs < rcv_size
 */
/*****************************************************************************/ 
static inline int
__max_size_at_offs(page_area_t * area, 
		   l4_offs_t offset, 
		   int rcv_size2,
		   l4_offs_t rcv_offs)
{
  l4_addr_t a;
  int max_align,max_size;

  a = (AREA_MAP_ADDR(area) + offset) | rcv_offs;
  if (a == 0)
    max_align = rcv_size2;
  else
    {
      max_align = l4util_bsf(a);
      if (max_align > rcv_size2)
	max_align = rcv_size2;
    }
  max_size = l4util_bsr(area->size - offset);

  LOGdL(DEBUG_MAP,"max align %d, max size %d",max_align,max_size);

  return (max_size < max_align ? max_size : max_align);
}

/*****************************************************************************/
/**
 * \brief  Create map flexpage.
 * 
 * \param  area          Memory area
 * \param  offset        Offset in memory area
 * \param  size2         Map area size (log2)
 * \param  rcv_size2     Remote receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 * \param  flags         Flags:
 *                       - \c L4DM_WRITE           create read/write fpage
 *                       - \c L4DM_MAP_PARTIAL     allow smaller fpages
 *                       - \c L4DM_MAP_MORE        allow bigger fpages
 *                       - \c L4DM_MEMPHYS_4MPAGES force 4MB-pages
 * \retval fpage         Flexpage descriptor
 *	
 * \return 0 on success (fpgae contains a valid flexpage descriptor), 
 *         error code otherwise:
 *         - \c -L4_EINVAL  invalid offset/size/receive window, could not 
 *                          create fpage which fits into given memory area
 *
 * Preconditions:
 * - offset/rcv_offs page aligned
 * - offset < area->size
 * - rcv_offs < rcv_size
 */
/*****************************************************************************/ 
static int
__build_map_fpage(page_area_t * area, 
		  l4_offs_t offset, 
		  int size2,
		  int rcv_size2, 
		  l4_offs_t rcv_offs, 
		  l4_uint32_t flags,
		  l4_snd_fpage_t * fpage)
{
  int fpage_size;
  l4_addr_t addr = AREA_MAP_ADDR(area) + offset;
  l4_addr_t map_start,map_end;

  /* get fpage size */
  if (flags & L4DM_MAP_MORE)
    {
      /* find the max. fpage which contains the page at the given offset. 
       * The start address of the fpage can be different than offset to
       * build a larger fpage. */
      fpage_size = __max_size(area,offset,rcv_size2,rcv_offs);
    }
  else
    {
      /* find the max. fpage with a start address at the given offset */
      fpage_size = __max_size_at_offs(area,offset,rcv_size2,rcv_offs);
      if (fpage_size > size2)
	fpage_size = size2;
    }

  map_start = addr & ~((1UL << fpage_size) - 1);
  map_end = map_start + (1UL << fpage_size);
  
  if ((fpage_size < size2) && !(flags & L4DM_MAP_PARTIAL))
    {
      /* _dice_corba_objed map area does not fit into page area */
#if DEBUG_ERRORS
      printf("memory area 0x%08x-0x%08x, offset 0x%08x\n (addr 0x%08x)\n",
             AREA_MAP_ADDR(area),AREA_MAP_ADDR(area) + area->size,
             offset,addr);
      printf("max. map area 0x%08x-0x%08x, size 0x%08lx (%d)\n",
             map_start,map_end,1UL << fpage_size,fpage_size);
      ERROR("DMphys: could not create fpage with size2 %d!",size2);
#endif
      return -L4_EINVAL;
    }

  if (flags & L4DM_MEMPHYS_4MPAGES)
    {
      /* requeseted 4MB-page, check */
      if (!dmphys_memmap_check_pagesize(map_start,1UL << fpage_size,
					DMPHYS_LOG2_4MPAGESIZE))
	{
	  /* no 4MB-pages */
	  ERROR("DMphys: no 4MB-pages at 0x%08x-0x%08x!",
		map_start,map_end);
	  return -L4_EINVAL;
	}
    }

  /* create fpage */
  if (flags & L4DM_WRITE)
    fpage->fpage = l4_fpage(map_start,fpage_size,L4_FPAGE_RW,L4_FPAGE_MAP);
  else
    fpage->fpage = l4_fpage(map_start,fpage_size,L4_FPAGE_RO,L4_FPAGE_MAP);
  fpage->snd_base = rcv_offs - (addr - map_start);

  LOGdL(DEBUG_MAP,"%s fpage 0x%08x-0x%08x\n" \
        "  size %d, snd_base 0x%08x",
        (flags & L4DM_WRITE) ? "rw" : "ro",map_start,map_end,
        fpage_size,fpage->snd_base);

#if MAP_FPAGE_PARANOIA
  /* check if fpage fits into memory area and fpage start address / size */
  if ((map_start < AREA_MAP_ADDR(area)) ||
      (map_end > (AREA_MAP_ADDR(area) + area->size)) ||
      ((map_start != addr) && !(flags & L4DM_MAP_MORE)) || 
      (fpage_size < L4_LOG2_PAGESIZE))
    {
      printf("memory area 0x%08x-0x%08x, offset 0x%08x\n",
             AREA_MAP_ADDR(area),AREA_MAP_ADDR(area) + area->size,offset);
      printf("map fpage 0x%08x-0x%08x, size %d\n",map_start,map_end,fpage_size);
      Panic("DMphys: fpage calculation failed!");
    }
#endif
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Unmap page area
 * 
 * \param  area          Page area descriptor
 */
/*****************************************************************************/ 
static void
__unmap_area(l4_addr_t adr, 
	     l4_size_t size)
{
  l4_addr_t map_addr = MAP_ADDR(adr);
  int addr_align,log2_size,fpage_size;

  while (size > 0)
    {
      LOGdL(DEBUG_UNMAP,"0x%08x, size %u (0x%08x)",map_addr,size,size);

      /* calculate the largest fpage we can unmap at address addr, 
       * it depends on the alignment of addr and of the size */
      addr_align = (map_addr == 0) ? 32 : l4util_bsf(map_addr);
      log2_size = l4util_bsr(size);
      fpage_size = (addr_align < log2_size) ? addr_align : log2_size;

      LOGdL(DEBUG_MAP,"align: addr %d, size %d, fpage %d",
            addr_align,log2_size,fpage_size);

      /* unmap page */
      l4_fpage_unmap(l4_fpage(map_addr,fpage_size,0,0),
		     L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);

      map_addr += (1UL << fpage_size);
      size -= (1UL << fpage_size);
    }
  
}

/*****************************************************************************
 *** DMphys internal API
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Unmap page area region
 * 
 * \param  addr          Area address
 * \param  size          Area size
 */
/*****************************************************************************/ 
void
dmphys_unmap_area(l4_offs_t addr, 
		  l4_size_t size)
{
  /* unmap */
  __unmap_area(addr,size);			       
}

/*****************************************************************************/
/**
 * \brief  Unmap page area list
 * 
 * \param  areas         Page area list head
 */
/*****************************************************************************/ 
void
dmphys_unmap_areas(page_area_t * areas)
{
  page_area_t * a = areas;

  /* unmap */
  while (a != NULL)
    {
      __unmap_area(a->addr,a->size);
      a = a->ds_next;
    }
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Map dataspace pages
 * 
 * \param  _dice_corba_obj       Flick _dice_corba_obj structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Map size 
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 * \param  flags         Access rights / flags
 *                       - \c L4DM_WRITE           map read/write fpage
 *                       - \c L4DM_MAP_PARTIAL     allow smaller fpages
 *                       - \c L4DM_MAP_MORE        allow bigger fpages
 *                       - \c L4DM_MEMPHYS_4MPAGES force 4MB-pages
 * \retval page          Flexpage descriptor
 * \retval _dice_corba_env           Flick exception structure (unused)
 *	
 * \return 0 on success (page contains a valid flexpage), error code 
 *         otherwise:
 *         - \c -L4_EINVAL       invalid dataspace id or map / receive window 
 *                               size
 *         - \c -L4_EINVAL_OFFS  invalid dataspace / receive window offset
 *         - \c -L4_EPERM        client has not the appropriate rights on the
 *                               dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_generic_map_component(CORBA_Object _dice_corba_obj,
                              l4_uint32_t ds_id,
                              l4_uint32_t offset,
                              l4_uint32_t size,
                              l4_uint32_t rcv_size2,
                              l4_uint32_t rcv_offs,
                              l4_uint32_t flags,
                              l4_snd_fpage_t *page,
                              CORBA_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;
  int ret,size2;
  page_area_t * area;
  l4_offs_t area_offset;

  /* set dummy fpage */
  page->snd_base = 0;
  page->fpage = l4_fpage(0,0,0,0);

  /* get dataspace descriptor, check access rights */
  ret = dmphys_ds_get_check_rights(ds_id,*_dice_corba_obj,
                                   flags & L4DM_RIGHTS_MASK,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace, id %u, caller %x.%x",
	      ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
      else
	{
	  printf("ds %u, caller %x.%x, rights 0x%04x, has 0x%04x\n",
                 ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread,
                 flags & L4DM_RIGHTS_MASK,
                 dmphys_ds_get_rights(ds,*_dice_corba_obj));
	  ERROR("DMphys: bad permissions");
	}
#endif
      return ret;
    }

  LOGdL(DEBUG_MAP,"caller %x.%x, ds %u\n" \
        "  offset 0x%08x, size 0x%08x, flags 0x%08x",
        _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,ds_id,
        offset,size,flags);

  /* round offset / size to pagesize */
  size = (((offset + size) + DMPHYS_PAGESIZE - 1) & DMPHYS_PAGEMASK) - 
    (offset & DMPHYS_PAGEMASK);
  offset &= DMPHYS_PAGEMASK;

  if (size == 0)
    /* nothing to do */
    return 0;

  size2 = l4util_bsr(size);
  if (((1UL << size2) != size) && !(flags & L4DM_MAP_PARTIAL))
    {
      if (flags & L4DM_MAP_MORE)
	size2++;
      else
	{
	  ERROR("DMphys: invalid map page size _dice_corba_objed: 0x%08x",size);
	  return -L4_EINVAL;
	}
    }
      
  /* find page area which contains offset */
  area = dmphys_ds_find_page_area(ds,offset,&area_offset);
  if (area == NULL)
    {
      ERROR("DMphys: invalid dataspace offset 0x%08x",offset);
      return -L4_EINVAL_OFFS;
    }

#if DEBUG_MAP
  printf("  aligned offset 0x%08x, size2 %d\n",offset,size2);
  printf("  page area 0x%08x-0x%08x, area offset 0x%08x\n",
         area->addr,area->addr + area->size,area_offset);
  printf("  rcv_size %u, rcv_offs 0x%08x\n",rcv_size2,rcv_offs);
#endif

  /* check receive window specification */
  if ((rcv_size2 < DMPHYS_LOG2_PAGESIZE) || (rcv_size2 > DMPHYS_AS_LOG2_SIZE))
    {
      ERROR("DMphys: invalid receive window (size %u)",rcv_size2);
      return -L4_EINVAL;
    }

  /* check offset in receive window */
  rcv_offs &= DMPHYS_PAGEMASK;
  if (rcv_offs >= (1ULL << rcv_size2))
    {
      ERROR("DMphys: invalid offset in receive window "
	    "(offset 0x%08x, size %u)",rcv_offs,rcv_size2);
      return -L4_EINVAL_OFFS;
    }
  
#if DEBUG_MAP
  printf("  aligned rcv_offs 0x%08x\n",rcv_offs);
#endif

  /* build map fpage */
  ret = __build_map_fpage(area,area_offset,size2,rcv_size2,rcv_offs,flags,page);
  if (ret < 0)
    {
      ERROR("DMphys: build map flexpage failed (%d)",ret);
      return ret;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Handle dataspace fault
 * 
 * \param  _dice_corba_obj       Flick _dice_corba_obj structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \retval page          Reply flexpage
 * \retval _dice_corba_env           Flick exception structure (unused)
 *	
 * \return 0 on success (page contains a valid flexpage), error code 
 *         otherwise:
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EINVAL_OFFS  invalid dataspace offset
 *         - \c -L4_EPERM        client has not the appropriate rights on the
 *                               dataspace
 * 
 * Build a flexpage for the page at the given dataspace offset.
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_generic_fault_component(CORBA_Object _dice_corba_obj,
                                l4_uint32_t ds_id,
                                l4_uint32_t offset,
                                l4_snd_fpage_t *page,
                                CORBA_Environment *_dice_corba_env)
{
  dmphys_dataspace_t * ds;
  int rw = offset & 2;  
  l4_uint32_t rights = (rw) ? (L4DM_READ | L4DM_WRITE) : L4DM_READ;
  int ret;
  page_area_t * area;
  l4_offs_t area_offset;
  l4_addr_t map_start;

  /* set dummy fpage */
  page->snd_base = 0;
  page->fpage = l4_fpage(0,0,0,0);

  /* get dataspace descriptor */
  ret = dmphys_ds_get_check_rights(ds_id,*_dice_corba_obj,rights,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace, id %u, client %x.%x",
	      ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
      else
	{
	  printf("ds %u, client %x.%x, rights 0x%04x, has 0x%04x\n",
                 ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread,
                 rights,dmphys_ds_get_rights(ds,*_dice_corba_obj));
	  ERROR("DMphys: bad permissions");
	}
#endif
      return ret;
    }

  LOGdL(DEBUG_FAULT,"client %x.%x, ds %u\n  offset 0x%08x, %s",
        _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,
        ds_id,offset,(rw) ? "rw" : "ro");

  /* round offset to pagesize */
  offset &= DMPHYS_PAGEMASK;

  /* find page area which contains offset */
  area = dmphys_ds_find_page_area(ds,offset,&area_offset);
  if (area == NULL)
    {
      ERROR("DMphys: invalid dataspace offset 0x%08x",offset);
      return -L4_EINVAL_OFFS;
    }

  /* create fpage */
  map_start = AREA_MAP_ADDR(area) + area_offset;
  if (rw)
    page->fpage = l4_fpage(map_start,DMPHYS_LOG2_PAGESIZE,
			   L4_FPAGE_RW,L4_FPAGE_MAP);
  else
    page->fpage = l4_fpage(map_start,DMPHYS_LOG2_PAGESIZE,
			   L4_FPAGE_RO,L4_FPAGE_MAP);
  page->snd_base = 0;

  LOGdL(DEBUG_FAULT,"\n  %s fpage 0x%08x-0x%08x",(rw) ? "rw" : "ro",
        map_start,map_start + DMPHYS_PAGESIZE);

  /* done */
  return 0;
}
