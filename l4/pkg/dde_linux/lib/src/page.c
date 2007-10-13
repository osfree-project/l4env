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
#include <asm/page.h>
#include <linux/mm.h>

/* local */
#include "__config.h"
#include "internal.h"

#include "fastcall.h"

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
unsigned long FASTCALL(__get_free_pages(unsigned int gfp_mask, unsigned int order))
{
  int error, pages;
  l4_addr_t page;
  l4_size_t size;

  l4_size_t tmp;
  l4dm_mem_addr_t dm_paddr;

  if (gfp_mask & GFP_DMA)
    LOGd(DEBUG_MSG, "Warning: No ISA DMA memory zone implemented.");

  size = L4_PAGESIZE << order;
  pages = 1 << order;

  LOGd(DEBUG_PALLOC, "requesting %d page(s) (pages)\n", pages);

  /* open and attach new dataspace */
  page = (l4_addr_t) \
    l4dm_mem_allocate_named(size,
                            L4DM_CONTIGUOUS | L4DM_PINNED |\
                            L4RM_MAP | L4RM_LOG2_ALIGNED,
                            "dde pages");
  if (!page)
    {
      LOGdL(DEBUG_ERRORS, "Error: allocating pages");
      return 0;
    }

  error = l4dm_mem_phys_addr((void *)page, 1, &dm_paddr, 1, &tmp);
  if (error != 1)
    {
      if (error>1 || !error)
        Panic("Ouch, what's that?");
      LOGdL(DEBUG_ERRORS, "Error: getting physical address (%d)", error);
      return 0;
    }

  /* address info */
  l4dde_add_region(page, dm_paddr.addr, size);

  LOGd(DEBUG_PALLOC, "allocated %d pages @ 0x%08lx (phys. 0x%08lx)",
       pages, page, dm_paddr.addr);

  return (unsigned long) page;
}

/** Allocate Free, Zeroed Memory Page
 * \ingroup mod_mm_p
 *
 * \param  gfp_mask  flags
 *
 * \return 0 on error, start address otherwise
 */
unsigned long FASTCALL(get_zeroed_page(unsigned int gfp_mask))
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
 */
void FASTCALL(free_pages(unsigned long addr, unsigned int order))
{
  int ret;
  l4_addr_t start;
  l4_size_t size;
  l4dm_dataspace_t ds;
  l4_offs_t offset;
  l4_threadid_t pager;

  ret = l4rm_lookup((void *)addr, &start, &size, &ds, &offset, &pager);

  if (ret == L4RM_REGION_DATASPACE) 
    {
      if (size == (L4_PAGESIZE << order))
        {
          l4dm_mem_release((const void *)addr);

          LOGd(DEBUG_PALLOC, "%s 2^%d pages released, addr= 0x%08lx, ",
               __FUNCTION__, order, addr);
        }
      else
        LOG_Error("size (%u) != mapped size (%u), skip release of addr=%08lx\n",
                  L4_PAGESIZE << order, size, addr);
    }
  else
    LOG_Error("Couldn't release memory of addr=%08lx\n", addr);
}

/** Release Memory Pages
 * \ingroup mod_mm_p
 *
 * \param  addr   start address of region
 * \param  order  log2(size)
 *
 * Release struct page * and calls free_page to
 * release page.
 */
void FASTCALL(__free_pages(struct page *page, unsigned int order))
{
  //sanity check
  if (page == NULL)
    {
      LOG_Error("attempt to free NULL page ...\n");
      return;
    }

  if (!put_page_testzero(page))
    {
      LOGd(DEBUG_PALLOC, "free page=%p denied, page->virtual=%p,"
           " because counter (%d) > 0 \n", page, page->virtual, page_count(page));
      return;
    }
  else
    {
      if (page->virtual != NULL)
        {
          LOGd(DEBUG_PALLOC, "free_pages: %p count=%d\n",
               page->virtual, page_count(page));

          free_pages((unsigned long)page->virtual, order);
        }
      else
        LOG_Error("attempt to free page->virtual==NULL, page=%p\n", page);
    }

  kfree(page);
}
