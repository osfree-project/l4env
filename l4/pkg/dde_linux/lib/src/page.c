/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/page.c
 * \brief  Page Allocation/Deallocation
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_mm
 * \defgroup mod_mm_p Page Allocation/Deallocation
 *
 * Page allocation just triggers mapping and deallocation triggers
 * unmapping. DDE does not keep track of these mappings (addresses and
 * sizes). So all knowledge remains in the drivers themselves.
 *
 * Memory allocations by get/free_pages are kmem!
 *
 * All these are Linux' %FASTCALL()s. That means for i386 and successors an
 * extra GCC attribute (regparm(3)) is added. I will keep it here.
 */

/* L4 */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/mm.h>

/* local */
#include "__config.h"
#include "internal.h"

/** Allocate Free Memory Pages
 * \ingroup mod_mm_p
 *
 * \param  gfp_mask  flags
 * \param  order     log2(size)
 *
 * \return 0 on error, start address otherwise
 *
 * Allocate 2^order physical contiguous pages. (inherently aligned!)
 *
 * \todo Physical alignment is not yet ensured.
 */
unsigned long __get_free_pages(unsigned int gfp_mask, unsigned int order)
{
  int error, pages;
  l4_addr_t page;
  l4_size_t size;

  l4_size_t tmp;
  l4dm_mem_addr_t dm_paddr;

  if (gfp_mask & GFP_DMA)
    DMSG("Warning: No ISA DMA memory zone implemented.\n");

  size = L4_PAGESIZE << order;
  pages = 1 << order;

#if DEBUG_PALLOC
  DMSG("requesting %d page(s) (pages)\n", pages);
#endif

  /* open and attach new dataspace */
  page = (l4_addr_t) \
    l4dm_mem_allocate_named(size,
                            L4DM_CONTIGUOUS | L4DM_PINNED |\
                            L4RM_MAP | L4RM_LOG2_ALIGNED,
                            "dde pages");
  if (!page)
    {
      ERROR("allocating pages");
      return 0;
    }

  error = l4dm_mem_phys_addr((void *)page, 1, &dm_paddr, 1, &tmp);
  if (error != 1)
    {
      if (error>1 || !error)
        Panic("Ouch, what's that?");
      ERROR("getting physical address (%d)", error);
      return 0;
    }

  /* address info */
  address_add_region(page, dm_paddr.addr, size);

#if DEBUG_PALLOC
  DMSG("allocated %d pages @ 0x%08x (phys. 0x%08x)\n",
       pages, page, dm_paddr.addr);
#endif

  return (unsigned long) page;
}

/** Allocate Free, Zeroed Memory Page
 * \ingroup mod_mm_p
 *
 * \param  gfp_mask  flags
 *
 * \return 0 on error, start address otherwise
 */
unsigned long get_zeroed_page(unsigned int gfp_mask)
{
  unsigned long page = __get_free_pages(gfp_mask, 0);

  if (!page)
    return 0;

  memset((void *) page, 0, L4_PAGESIZE);

  return page;
}

/** Release Memory Pages
 * \ingroup mod_mm_p
 *
 * \param  addr   start address of region
 * \param  order  log2(size)
 *
 * Release 2^order contiguous pages.
 *
 * \todo implementation
 */
void free_pages(unsigned long addr, unsigned int order)
{
  Error("%s for 2^%d pages not implemented", __FUNCTION__, order);
}
