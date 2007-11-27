/*
 * \brief   Page allocation
 * \author  Christian Helmuth
 * \date    2007-01-22
 *
 * In Linux 2.6 this resides in mm/page_alloc.c.
 *
 * This implementation is far from complete as it does not cover "struct page"
 * emulation. In Linux, there's an array of structures for all pages. In
 * particular, iteration works for this array like:
 *
 *   struct page *p = alloc_pages(3); // p refers to first page of allocation
 *   ++p;                             // p refers to second page
 *
 * There may be more things to cover and we should have a deep look into the
 * kernel parts we want to reuse. Candidates for problems may be file systems,
 * storage (USB, IDE), and video (bttv).
 */

/* Linux */
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/pagevec.h>
#include <asm/page.h>

/* DDEKit */
#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/assert.h>
#include <l4/dde/ddekit/panic.h>

#include "local.h"

unsigned long max_low_pfn;
unsigned long min_low_pfn;
unsigned long max_pfn;

/*******************
 ** Configuration **
 *******************/

#define DEBUG_PAGE_ALLOC 0


struct page * fastcall __alloc_pages(gfp_t gfp_mask, unsigned int order,
                                     struct zonelist *zonelist)
{
	WARN_UNIMPL;

	return 0;
}


fastcall unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
	ddekit_log(DEBUG_PAGE_ALLOC, "gfp_mask=%x order=%d (%d bytes)",
	           gfp_mask, order, PAGE_SIZE << order);

	Assert(gfp_mask != GFP_DMA);
	void *p = ddekit_large_malloc(PAGE_SIZE << order);

	return (unsigned long)p;
}


fastcall unsigned long get_zeroed_page(gfp_t gfp_mask)
{
	unsigned long p = __get_free_pages(gfp_mask, 0);

	if (p) memset((void *)p, 0, PAGE_SIZE);

	return (unsigned long)p;
}


void fastcall free_hot_page(struct page *page)
{
	WARN_UNIMPL;
}


fastcall void __free_pages(struct page *page, unsigned int order)
{
	WARN_UNIMPL;
}

void __pagevec_free(struct pagevec *pvec)
{
	WARN_UNIMPL;
}

int get_user_pages(struct task_struct *tsk, struct mm_struct *mm,
                   unsigned long start, int len, int write, int force,
                   struct page **pages, struct vm_area_struct **vmas)
{
	WARN_UNIMPL;
	return 0;
}

/**
 * ...
 *
 * XXX order may be larger than allocation at 'addr' - it may comprise several
 * allocation via __get_free_pages()!
 */
fastcall void free_pages(unsigned long addr, unsigned int order)
{
	ddekit_log(DEBUG_PAGE_ALLOC, "addr=%p order=%d", (void *)addr, order);

	ddekit_large_free((void *)addr);
}


unsigned long __pa(volatile void *addr)
{
	return ddekit_pgtab_get_physaddr((void *)addr);
}

void *__va(unsigned long addr)
{
	printk("__va not implemented!\n");
	BUG();
	return NULL;
}


int set_page_dirty_lock(struct page *page)
{
	WARN_UNIMPL;
	return 0;
}
