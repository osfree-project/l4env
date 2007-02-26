/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/address.c
 * \brief  Memory Address Conversion and Region Handling
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
 * \defgroup mod_mm_addr Address Conversion
 *
 * Linux' %__va()/%__pa() macro replacements.
 *
 * We look in our local table(s) to save IPC costs. If this fails nasty things
 * are going on and we Panic.
 *
 * The current implementation is \e slow, iteration based. Optimization is
 * possible and should be done soon using a \e hashing-like scheme.
 *
 * Another straight-forward solution was mentioned by Jork: configure only one
 * big kmem region and save traversing a list. This can be done be setting
 * \c KMEM_RSIZE.
 *
 * \todo error handling
 *
 * \todo what about I/O pages?
 *
 * \todo implement remove_region()
 *
 * \todo introduce region names
 *
 * \krishna Ouch, if "struct conv_region" is vmalloc'ed, we may get a pfault
 * on address conversion - that IS slow.
 */

/* L4 */
#include <l4/env/errno.h>

#include <l4/dde_linux/dde.h>

/* local */
#include "__config.h"
#include "internal.h"

/** \name Module Variables
 * @{ */
/** region list for memory management */
static struct dde_region *conv = NULL;

/** @} */
/** Address conversion region addition.
 *
 * \param  va    virtual start address
 * \param  pa    physical start address
 * \param  size  region size
 */
void l4dde_add_region(l4_addr_t va, l4_addr_t pa, l4_size_t size)
{
  dde_add_region(&conv, va, pa, size);
}

/** Address conversion region removal.
 *
 * \param  va    virtual start address
 * \param  size  region size
 */
void l4dde_remove_region(l4_addr_t va, l4_size_t size)
{
  dde_remove_region(&conv, va, 0, size);
}

/** Convert physical to virtual memory address.
 * \ingroup mod_mm_addr
 *
 * \param  paddr  physical memory address
 *
 * \return virtual address
 */
void *__va(volatile unsigned long paddr)
{
  struct dde_region *p;
  void *pp;

  /* look in conversion table/list */
  p = conv;
  while (p)
    {
      if ((p->pa <= paddr) && (p->pa + p->size > paddr))
        {
          pp = (void *) (p->va + (paddr - p->pa));
#if DEBUG_ADDRESS
          LOG("__va(0x%lx) => %p\n", paddr, pp);
#endif
          return pp;
        }
      p = p->next;
    }

  Panic("no virtual address found for 0x%lx", paddr);
  return NULL;
}

/** Convert virtual to physical memory address.
 * \ingroup mod_mm_addr
 *
 * \param  vaddr  virtual memory address
 *
 * \return physical address
 */
unsigned long __pa(volatile void *vaddr)
{
  struct dde_region *p;
  unsigned long pp;
  unsigned long addr = (unsigned long) vaddr;

  /* look in conversion table/list */
  p = conv;
  while (p)
    {
      if ((p->va <= addr) && (p->va + p->size > addr))
        {
          pp = p->pa + (addr - p->va);
#if DEBUG_ADDRESS
          LOG("__pa(0x%lx) => 0x%lx\n", addr, pp);
#endif
          return pp;
        }
      p = p->next;
    }

  Panic("no physical address found for %p", vaddr);
  return 0;
}
