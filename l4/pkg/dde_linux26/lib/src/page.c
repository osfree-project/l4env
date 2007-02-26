/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/src/page.c
 *
 * \brief	Page Allocation/Deallocation
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Original by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
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
/*****************************************************************************/

/* L4 */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/mm.h>
#include <linux/linkage.h>

/* local */
#include "__config.h"
#include "__macros.h"
#include "internal.h"

/*****************************************************************************/
/** Allocate Free Memory Pages
 * \ingroup mod_mm_p
 *
 * \param  gfp_mask	flags
 * \param  order	log2(size)
 *
 * \return 0 on error, start address otherwise
 *
 * Allocate 2^order physical contiguous pages. (inherently aligned!)
 *
 * \todo Physical alignment is not yet ensured.
 */
/*****************************************************************************/
unsigned long fastcall __get_free_pages(unsigned int gfp_mask, unsigned int order)
{
  int error, pages;
  l4_addr_t page;
  l4_size_t size;

  l4_size_t tmp;
  l4dm_mem_addr_t dm_paddr;

  if (gfp_mask & GFP_DMA)
    LOG("Warning: No ISA DMA implemented.");

  size = L4_PAGESIZE << order;
  pages = 1 << order;

  LOGd(DEBUG_PALLOC, "requesting %d page(s) (pages)", pages);

  /* open and attach new dataspace */
  page = (l4_addr_t) \
    l4dm_mem_allocate_named(size,
			    L4DM_CONTIGUOUS | L4DM_PINNED |\
			    L4RM_MAP | L4RM_LOG2_ALIGNED,
			    "dde pages");
  if (!page)
    {
      LOG_Error("allocating pages");
      return 0;
    }

  error = l4dm_mem_phys_addr((void *)page, 1, &dm_paddr, 1, &tmp);
  if (error != 1)
    {
      if (error>1 || !error)
	Panic("Ouch, what's that?");
      LOG_Error("getting physical address (%d)", error);
      return 0;
    }

  /* address info */
  address_add_region(page, dm_paddr.addr, size);

  LOGd(DEBUG_PALLOC, "allocated %d pages @ 0x%08x (phys. 0x%08x)",
       pages, page, dm_paddr.addr);

  return (unsigned long) page;
}

/*****************************************************************************/
/** Allocate Free, Zeroed Memory Page
 * \ingroup mod_mm_p
 *
 * \param  gfp_mask	flags
 *
 * \return 0 on error, start address otherwise
 */
/*****************************************************************************/
unsigned long fastcall get_zeroed_page(unsigned int gfp_mask)
{
  unsigned long page = __get_free_pages(gfp_mask, 0);

  if (!page)
    return 0;

  memset((void *) page, 0, L4_PAGESIZE);

  return page;
}

/*****************************************************************************/
/** Release Memory Pages
 * \ingroup mod_mm_p
 *
 * \param  addr
 * \param  order	log2(size)
 *
 * Release 2^order contiguous pages.
 *
 * \todo implementation
 */
/*****************************************************************************/
void fastcall free_pages(unsigned long addr, unsigned int order)
{
  LOG_Error("%s for 2^%d pages not implemented", __FUNCTION__, order);
}
