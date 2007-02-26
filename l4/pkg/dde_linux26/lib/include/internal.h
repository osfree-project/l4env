/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/include/internal.h
 *
 * \brief	Internal Helpers / Interfaces
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Original by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/**
 * \defgroup internals Internals
 *
 */

#ifndef _LIBDDE_INTERNAL_H
#define _LIBDDE_INTERNAL_H

#include <l4/util/macros.h>
#include <linux/vmalloc.h>
#include "__config.h"

/* Address Conversion Helpers */
void address_add_region(l4_addr_t, l4_addr_t, l4_size_t);
void address_remove_region(l4_addr_t, l4_size_t);

/* Region Handling Helpers */

/** region data type */
struct dde_region
{
  l4_addr_t va;			/**< virtual start address */
  l4_addr_t pa;			/**< physical start address */
  l4_size_t size;		/**< region size */
  struct dde_region *next;	/**< next region pointer */
};

/*****************************************************************************/
/** Internal Region Addition
 * \ingroup internals
 *
 * \param  head		head of region list
 * \param  va		virtual start address
 * \param  pa		physical start address
 * \param  size		region size
 *
 * \krishna do we have overlapping regions in any case? we do not test for
 * these!
 * */
/*****************************************************************************/
static inline void dde_add_region(struct dde_region **head,
				  l4_addr_t va, l4_addr_t pa, l4_size_t size)
{
  struct dde_region *p = vmalloc(sizeof(struct dde_region));

  p->va = va;
  p->pa = pa;
  p->size = size;
  p->next = *head;

  *head = p;

  LOGd(DEBUG_MSG, "new dde_region [0x%08lx, 0x%08lx, %d]\n", va, pa, size);
}

/*****************************************************************************/
/** Internal Region Removal
 * \ingroup internals
 *
 * \param  head		head of region list
 * \param  va		virtual start address
 * \param  pa		physical start address
 * \param  size		region size
 *
 * This removes the region described by \a va/pa and \a size from list \a head.
 *
 * \krishna hmm, could be complicated if we free not the whole allocated region
 *
 * -# search right node
 * -# remove node resp. split it up into more nodes
 */
/*****************************************************************************/
static inline void dde_remove_region(struct dde_region **head,
				     l4_addr_t va, l4_addr_t pa, l4_size_t size)
{
#if 1
  LOGL("not implemented yet\n");
#else
  struct dde_region *p = *head;

  LOG("remove conv_region [0x%08lx, %d]\n", va, size);
#endif
}

#endif /* !_LIBDDE_INTERNAL_H */
