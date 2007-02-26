/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/src/mmio.c
 *
 * \brief	(Fake) Memory Mapped I/O
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Original by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/sys/types.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/mm.h>

/* local includes */
#include "__config.h"
#include "__macros.h"

/******************************************************************************/
/*!
 * \brief  Linux' ???
 * \ingroup grp_misc
 *
 * \param virt_addr	user virtual address where the mapping should start
 * \param phys_addr	physical address to which the mapping maps
 * \param size		size of vma in bytes
 * \param prot		protection for the new vma
 *
 * \todo Hmm, what was the idea of that?!
 ******************************************************************************/
int remap_page_range(struct vm_area_struct *vma, unsigned long virt_addr, 
		     unsigned long phys_addr, unsigned long size, pgprot_t prot)
{
  return -ENOMEM;
}

/* mmap() / remap_page_range() / vm_area_struct.nopage() discussion

   (mostly from Rubini: Linux Device Drivers)

   - logical addresses are not available for high memory. thus kernel functions
   that deal with memory are increasingly using pointers to "struct page"
   instead.

   - "struct page" includes a "void *virtual" member (low memory is always
   mapped high memory not)

     - kernel virtual address of the page or

     - NULL if not mapped

   - virt_to_page() return "struct page" associated to logical address

   - page_address() return virtual addres of "struct page"

   - kmap() return virtual address of any page (if not mapped mapping is done
   before return)

   - kunmap() frees special kmap() mappings

   - virtual memory areas (vma) is a higher-level mechanism to handle the way a
   process sees its memory

   - a driver that implements mmap() needs to fill a vma structure in the
   address space of the process mapping the device

   - "struct vm_area_struct"

   - "struct vm_operations_struct"
     ... open/close/unmap/protect/sync/nopage/swapout

   - 

 **/
