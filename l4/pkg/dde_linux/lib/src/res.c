/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/src/res.c
 *
 * \brief	I/O Resource Management
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/** \ingroup mod_common
 * \defgroup mod_res I/O Resource Management
 *
 * This module emulates the I/O resource management inside the Linux kernel.
 *
 * It is mostly a wrapper to libio request/release functions for I/O port and
 * memory regions with additional bookkeeping of allocations. The
 * ioremap()/iounmap() interface is also provided by this module.
 *
 * Requirements: (additionally to \ref pg_req)
 * 
 * - initialized libio
 *
 * \todo I/O memory as dataspaces?!
 *
 * \todo region handling
 *
 * -# put regions in conversion list via address_add_region() too
 * -# hold only 1 region list (\c address.c) and call \c address_*() and \c
 * _va()
 * */
/*****************************************************************************/

/* L4 */
#include <l4/generic_io/libio.h>

#include <l4/dde_linux/dde.h>
#include <l4/env/errno.h>

/* Linux */
#include <linux/ioport.h>

#include <l4/log/l4log.h>

/* local */
#include "__config.h"
#include "__macros.h"
#include "internal.h"

/*****************************************************************************/
/** 
 * \name Module Variables
 * @{ */
/*****************************************************************************/
/** I/O memory region list */
static struct dde_region *regions = NULL;

/** @} */
/*****************************************************************************/
/** Allocate I/O port region.
 * \ingroup mod_res
 *
 * \param  start	begin of region
 * \param  n		length of region
 * \param  name		name of requester
 *
 * \krishna hopefully nobody uses return values
 * \bug Return value is a bogus pointer.
 */
/*****************************************************************************/
struct resource *request_region(unsigned long start, unsigned long n,
				const char *name)
{
  int err;

#if DEBUG_RES_TRACE
  INFO("io_addr=0x%04x, size=%d, name=\"%s\"\n", start, n, name);
#endif
  err = l4io_request_region((l4_addr_t) start, (l4_size_t) n);

  if (err)
    {
      ERROR("in l4io_request_region(0x%04lx, %ld) (%d)", start, n, err);
      return 0;
    }

  return (struct resource *) 1;
}

/*****************************************************************************/
/** Allocate I/O memory region.
 * \ingroup mod_res
 *
 * \param  start	begin of region
 * \param  n		length of region
 * \param  name		name of requester
 *
 * \krishna hopefully nobody uses return values
 * \bug Return value is a bogus pointer.
 */
/*****************************************************************************/
struct resource *request_mem_region(unsigned long start, unsigned long n,
				    const char *name)
{
  l4_addr_t vaddr;
  l4_addr_t offset;

#if DEBUG_RES_TRACE
  INFO("phys_addr=%p, size=%d, name=\"%s\"\n", start, n, name);
#endif
  vaddr = l4io_request_mem_region((l4_addr_t) start, (l4_size_t) n, &offset);

  if (!vaddr)
    {
      ERROR("in l4io_request_mem_region(%p, %ld)", (void*)start, n);
      return 0;
    }
  /* keep region info */
  dde_add_region(&regions, vaddr+offset, start, n);

  return (struct resource *) 1;
}

/*****************************************************************************/
/** Release I/O port region.
 * \ingroup mod_res
 *
 * \param  start	begin of region
 * \param  n		length of region
 */
/*****************************************************************************/
void release_region(unsigned long start, unsigned long n)
{
  int err;

#if DEBUG_RES_TRACE
  INFO("io_addr=0x%04x, size=n\n", start, n);
#endif
  err = l4io_release_region((l4_addr_t) start, (l4_size_t) n);

  if (err)
    Error("release_region(0x%04lx, %ld) failed (%d)", start, n, err);
}

/*****************************************************************************/
/** Release I/O memory region.
 * \ingroup mod_res
 *
 * \param  start	begin of region
 * \param  n		length of region
 */
/*****************************************************************************/
void release_mem_region(unsigned long start, unsigned long n)
{
  int err;

#if DEBUG_RES_TRACE
  INFO("phys_addr=%p, size=%d\n", start, n);
#endif
  err = l4io_release_mem_region((l4_addr_t) start, (l4_size_t) n);

  if(err)
    {
      ERROR("release_mem_region(%p, %ld) failed (%d)", (void*)start, n, err);
      return;
    }

  /* keep region info */
  dde_remove_region(&regions, 0, start, n);
}

/** Release any resource
 * \ingroup mod_res
 *
 * \param  resource	The resource to free
 *
 * \todo implementation
 */
int release_resource(struct resource *res)
{
  Error("%s not implemented", __FUNCTION__);

  return -L4_EINVAL;
}

/*****************************************************************************/
/** Check I/O port region availability.
 * \ingroup mod_res
 *
 * \param  start	begin of region
 * \param  n		length of region
 *
 * \return always 0
 *
 * We provide no support for this.
 */
/*****************************************************************************/
int check_region(unsigned long start, unsigned long n)
{
  return 0;
}

/*****************************************************************************/
/** Check I/O memory region availability.
 * \ingroup mod_res
 *
 * \param  start	begin of region
 * \param  n		length of region
 *
 * \return always 0
 *
 * We provide no support for this.
 */
/*****************************************************************************/
int check_mem_region(unsigned long start, unsigned long n)
{
  return 0;
}

/*****************************************************************************/
/** Remap I/O memory into kernel address space.
 * \ingroup mod_res
 *
 * \param phys_addr	begin of physical address range
 * \param size		size of physical address range
 *
 * \return virtual start address of mapped range
 *
 * Here no real mapping is done. Only the virtual address is returned.
 */
/*****************************************************************************/
void *ioremap(unsigned long phys_addr, unsigned long size)
{
  struct dde_region *p;
  void *pp;

#if DEBUG_RES_TRACE
  INFO("phys_addr=%p, size=%d\n", phys_addr, size);
#endif

#if !LATER
#warning it is not later
  /* look in I/O region table/list */
  p = regions;
  while (p)
    {
      if ((p->pa <= phys_addr) && 
	  (p->pa + p->size >= phys_addr + size))
	{
	  pp = (void *) (p->va + (phys_addr - p->pa));
#if DEBUG_RES
	  DMSG("ioremap: %p => %p\n", (void*)phys_addr, pp);
#endif
	  return pp;
	}
      p = p->next;
    }
#else /* LATER, whatever this means */
  if ((pp=(void*)__va(phys_addr)) && __va(phys_addr+size))
    {
#if DEBUG_RES
      DMSG("ioremap: <%p-%p> => <%p-%p>\n",
	   phys_addr, phys_addr+size, pp, pp+size);
#endif
      return pp;
    }
#endif /* LATER */

  LOG_Error("no ioremap address found for %p", (void*)phys_addr);
  return (void *) 0;
}

/*****************************************************************************/
/** Remap I/O memory into kernel address space (no cache).
 * \ingroup mod_res
 *
 * \param phys_addr	begin of physical address range
 * \param size		size of physical address range
 *
 * \return virtual start address of mapped range
 *
 * Here no real mapping is done. Only the virtual address is returned (by
 * calling ioremap()).
 */
/*****************************************************************************/
void *ioremap_nocache (unsigned long phys_addr, unsigned long size)
{
#if DEBUG_RES_TRACE
  INFO("phys_addr=%p, size=%d\n", phys_addr, size);
#endif

  return ioremap(phys_addr, size);
}

/*****************************************************************************/
/** Unmap I/O memory from kernel address space.
 * \ingroup mod_res
 *
 * \param addr		virtual start address
 *
 * Do nothing.
 */
/*****************************************************************************/
void iounmap(void *addr)
{
#if DEBUG_RES_TRACE
  INFO("addr=%p\n", addr);
#endif
}
