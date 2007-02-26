/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/mm.c
 * \brief  Memory Management
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_common
 * \defgroup mod_mm Memory Management
 *
 * This module emulates the Memory Management subsystem inside the Linux
 * kernel.
 *
 * Linux DDE manages two OSKit <em>LMM pools</em> for memory allocations - one
 * for kmalloc() (kmem) and one for vmalloc() (vmem) allocations. Each pool
 * consists of one ore more <em>LMM regions</em>.
 *
 * Malloc functions have to keep track of chunk sizes and store them in the
 * first dword of the chunk.
 *
 * Configuration:
 *
 * - setup #MM_KREGIONS to change number of kernel memory regions (default 1!)
 * - maximum size of pools is configured via parameters of l4dde_mm_init()
 */

/* L4 */
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/names/libnames.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/util/list_alloc.h>
#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/slab.h>
#include <linux/vmalloc.h>

/* local */
#include "__config.h"
#include "internal.h"

/** mm/memory.c */

unsigned long num_physpages = 0;

/** \name Module Variables
 * @{ */

/** initialization flag */
static int _initialized = 0;

/* kernel memory - kmem */
/** kmem pool */
static l4la_free_t *kpool = L4LA_INITIALIZER;
/** kmem region size */
static unsigned int kregion_size;
/** kmem region counter */
static unsigned int kcount = MM_KREGIONS - 1;
/** protect access to kmem pool */
static l4lock_t klock = L4LOCK_UNLOCKED_INITIALIZER;

/* virtual memory - vmem */
/** vmem pool */
static l4la_free_t *vpool = L4LA_INITIALIZER;
/** protect access to vmem pool */
static l4lock_t vlock = L4LOCK_UNLOCKED_INITIALIZER;

/** @} */
/** Simulated LMM Callback on Memory Shortage (kmem)
 *
 * \param  size   size of memory chunk needed
 *
 * \return 0 on success, negative error code otherwise
 *
 * As long as maximum region number is not exceeded new <em>physical
 * contiguous</em> dataspaces are requested from a <em>dataspace manager</em>
 * and attached to appropriate regions in address space by the <em>L4 Region
 * Mapper</em>. The region is then added as new LMM region to the kmem LMM pool.
 *
 * \p size is rounded up to multiples of \c KMEM_RSIZE.
 *
 * I removed the unlock/lock around page requesting that was in the original
 * OSKit implementation's \c MORECORE() macro (\c clientos/mem.c). Hope it's
 * still fully functioning.
 *
 */
static __inline__ int __more_kcore(l4_size_t size)
{
  int error;
  l4_addr_t kaddr;

  l4_size_t tmp;
  l4dm_mem_addr_t dm_paddr;

  if (!(kcount) || (size > kregion_size))
    {
      LOGdL(DEBUG_ERRORS, "Error: out of memory (kmem)");
      return -L4_ENOMEM;
    }

  LOGd(DEBUG_MALLOC, "requesting %d bytes (kmem)", size);

  /* open and attach new dataspace */
  kaddr = (l4_addr_t) \
    l4dm_mem_allocate_named(kregion_size,
                            L4DM_CONTIGUOUS | L4DM_PINNED | L4RM_MAP,
                            "dde kmem");
  if (!kaddr)
    {
      LOGdL(DEBUG_ERRORS, "Error: allocating kmem");
      return -L4_ENOMEM;
    }

  error = l4dm_mem_phys_addr((void *) kaddr, 1, &dm_paddr, 1, &tmp);
  if (error != 1)
    {
      if (error>1 || !error)
        Panic("Ouch, what's that?");
      LOGdL(DEBUG_ERRORS, "Error: getting physical address (%d)", error);
      return error;
    }

  LOGd(DEBUG_MALLOC, "adding %d Bytes (kmem) @ 0x%08x (phys. 0x%08x) region %d",
       size, kaddr, dm_paddr.addr, kcount);

  /* add new region */
  l4la_free(&kpool, (void*)kaddr, size);

  /* address info */
  address_add_region(kaddr, dm_paddr.addr, size);

  return 0;
}

/** Simulated LMM Callback on Memory Shortage (vmem)
 *
 * \param  size  size of memory chunk needed
 *
 * \return -L4_ENOMEM
 *
 * This always returns "Out of memory" as we leave mappings in vmem region to
 * l4rm and dataspace managers.
 */
static __inline__ int __more_vcore(l4_size_t size)
{
  LOGdL(DEBUG_ERRORS, "Error: out of memory (vmem)");
  return -L4_ENOMEM;
}

/** kmem Allocation
 * \ingroup mod_mm
 *
 * \param  size  size of memory chunk
 * \param  gfp   flags
 *
 * This is Linux' %kmalloc().
 *
 * \return start address of allocated chunk; NULL on error
 */
void *kmalloc(size_t size, int gfp)
{
  l4_uint32_t *chunk;

  if (gfp & GFP_DMA)
    LOGd(DEBUG_MSG, "Warning: No ISA DMA implemented.");

  /* mutex pool access */
  l4lock_lock(&klock);

  size += sizeof(size_t);
  while (!(chunk = l4la_alloc(&kpool, size, 0)))
    {
      if (__more_kcore(size))
        {
#if DEBUG_MALLOC
          LOG("failed to allocate %d bytes (kmem)", size);
          l4la_dump(&kpool);
#endif
          return NULL;
        }
    }
  *chunk = size;

  LOGd(DEBUG_MALLOC_EACH, "allocated %d bytes @ %p (kmem)", *chunk, chunk);

  l4lock_unlock(&klock);

  return ++chunk;
}

/** kmem Deallocation
 * \ingroup mod_mm
 *
 * \param  addr  start address of memory chunk to free
 *
 * This is Linux' %kfree().
 */
void kfree(const void *addr)
{
  l4_uint32_t *chunk = (l4_uint32_t *) addr - 1;

  if (!addr)
    return;

  /* mutex pool access */
  l4lock_lock(&klock);

  LOGd(DEBUG_MALLOC_EACH, "freeing %d bytes @ %p (kmem)", *chunk, chunk);

  l4la_free(&kpool, chunk, *chunk);

  l4lock_unlock(&klock);
}

unsigned int nr_free_pages(void)
{
  return l4la_avail(&kpool) >> PAGE_SHIFT;
}

/** vmem Allocation
 * \ingroup mod_mm
 *
 * \param  size  size of memory chunk
 *
 * \return start address of allocated chunk; NULL on error
 *
 * This is Linux' %vmalloc().
 */
void *vmalloc(unsigned long size)
{
  l4_uint32_t *chunk;

  /* mutex pool access */
  l4lock_lock(&vlock);

  size += sizeof(size_t);
  while (!(chunk = l4la_alloc(&vpool, size, 0)))
    {
      if (__more_vcore(size))
        {
#if DEBUG_MALLOC
          LOG("failed to allocate %ld bytes (vmem)", size);
          l4la_dump(&vpool);
#endif
          return NULL;
        }
    }
  *chunk = size;

  LOGd(DEBUG_MALLOC_EACH, "allocated %d bytes @ %p (vmem)", *chunk, chunk);

  l4lock_unlock(&vlock);

  return ++chunk;
}

/** vmem Deallocation
 * \ingroup mod_mm
 *
 * \param  addr  start address of memory chunk to release
 *
 * This is Linux' %vfree().
 */
void vfree(void *addr)
{
  l4_uint32_t *chunk = (l4_uint32_t *) addr - 1;

  if (!addr)
    return;

  /* mutex pool access */
  l4lock_lock(&vlock);

  l4la_free(&vpool, chunk, *chunk);

  LOGd(DEBUG_MALLOC_EACH, "freed %d bytes @ %p (vmem)", *chunk, chunk);

  l4lock_unlock(&vlock);
}

/*****************************************************************************/
/** Setup kmem pool. */
/*****************************************************************************/
static int __setup_kmem(unsigned int *max, l4_addr_t *addr)
{
  unsigned int size = *max;
  int error;

  l4_size_t tmp;
  l4dm_mem_addr_t dm_paddr;

  kregion_size = size / MM_KREGIONS;
  LOGd(DEBUG_MALLOC, "size/regions = %d", kregion_size);

  kregion_size = (kregion_size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  LOGd(DEBUG_MALLOC, "rsize (mod PAGESIZE) = %d", kregion_size);

  if (kregion_size * MM_KREGIONS < size)
    {
      kregion_size += L4_PAGESIZE;
      LOGd(DEBUG_MALLOC, "new rsize = %d\n", kregion_size);
    }

  LOGd(DEBUG_MALLOC, "kregion_size = 0x%x regions = %d", 
       kregion_size, MM_KREGIONS);

  /* open and attach initial dataspace */
  *addr = (l4_addr_t) \
    l4dm_mem_allocate_named(kregion_size,
                            L4DM_CONTIGUOUS | L4DM_PINNED | L4RM_MAP,
                            "dde kmem");
  if (!*addr) return -L4_ENOMEM;

  error = l4dm_mem_phys_addr((void *)*addr, 1, &dm_paddr, 1, &tmp);
  if (error != 1)
    {
      if (error>1 || !error)
        Panic("Ouch, what's that?");
      LOGdL(DEBUG_ERRORS, "Error: getting physical address (%d)", error);
      return error;
    }

  LOGd(DEBUG_MALLOC, "adding %d Bytes (kmem) @ 0x%08x (phys. 0x%08x) region 0",
       kregion_size, *addr, dm_paddr.addr);
  
  /* add free memory area to region/pool */
  l4la_free(&kpool, (void *) *addr, kregion_size);

  /* address info */
  address_add_region(*addr, dm_paddr.addr, kregion_size);

  *max = kregion_size * MM_KREGIONS;
  return 0;
}

/*****************************************************************************/
/** Setup vmem pool. */
/*****************************************************************************/
static int __setup_vmem(unsigned int *max, l4_addr_t *addr)
{
  unsigned int size = *max;

  /* round */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* open vmem dataspace */
  *addr = (l4_addr_t) l4dm_mem_allocate_named(size, L4RM_MAP, "dde vmem");
  if (!*addr) return -L4_ENOMEM;

  /* add free memory area */
  l4la_free(&vpool, (void *) *addr, size);

  *max = size;
  return 0;
}

/** Initalize LMM pools and initial regions.
 * \ingroup mod_mm
 *
 * \param  max_vsize  max size of virtual memory allocations
 * \param  max_ksize  max size of kernel memory allocations
 *
 * \return 0 on success, negative error code otherwise
 *
 * Sizes are rounded up to multiples of page size.
 *
 * kmem pool is divided into MM_KREGIONS regions of same size. The total size
 * will be \c max_ksize (or greater because of rounding/page size). vmem pool
 * has only one region of \c max_vsize.
 */
int l4dde_mm_init(unsigned int max_vsize, unsigned int max_ksize)
{
  int error;
  l4_addr_t vaddr, kaddr;

  char *vsize_str;
  char *ksize_str;

  if (_initialized)
    return 0;

  /* setup virtual memory pool */
  if ((error=__setup_vmem(&max_vsize, &vaddr)))
    {
      LOGdL(DEBUG_ERRORS, "Error: setting up vmem: %d (%s)", 
            error, l4env_strerror(-error));
      return error;
    }

  /* setup kernel memory pool */
  if ((error=__setup_kmem(&max_ksize, &kaddr)))
    {
      LOGdL(DEBUG_ERRORS, "Error: setting up kmem %d (%s)", 
            error, l4env_strerror(-error));
      return error;
    }

  /* tell memory size in pages */
  num_physpages = max_ksize / L4_PAGESIZE;

  /* print pretty message */
  if (max_vsize > 8 * 1024 * 1024)
    {
      max_vsize /= (1024 * 1024);
      vsize_str = "MB";
    }
  else if (max_vsize > 8 * 1024)
    {
      max_vsize /= 1024;
      vsize_str = "kB";
    }
  else
    vsize_str = "Byte";
  if (max_ksize > 8 * 1024 * 1024)
    {
      max_ksize /= (1024 * 1024);
      ksize_str = "MB";
    }
  else if (max_ksize > 8 * 1024)
    {
      max_ksize /= 1024;
      ksize_str = "kB";
    }
  else
    ksize_str = "Byte";
  LOG("Using ...\n"
      "  %d %s at 0x%08x (vmem)\n"
      "  %d %s in %d regions (kmem)",
      max_vsize, vsize_str, vaddr, max_ksize, ksize_str, MM_KREGIONS);

#if DEBUG_MALLOC
  {
    int debug;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;
    l4_threadid_t dummy;

    debug = l4rm_lookup((void*)vaddr, &map_addr, &map_size, &ds, &offset, &dummy);
    if (debug != L4RM_REGION_DATASPACE)
      Panic("l4rm_lookup failed (%d)", debug);
    LOG("vmem: ds={%3u, "l4util_idfmt"} offset=%d map_addr=0x%08x map_size=%d",
        ds.id, l4util_idstr(ds.manager), offset, map_addr, map_size);
    
    debug = l4rm_lookup((void*)kaddr, &map_addr, &map_size, &ds, &offset, &dummy);
    if (debug != L4RM_REGION_DATASPACE)
      Panic("l4rm_lookup failed (%d)", debug);
    LOG("kmem: ds={%3u, "l4util_idfmt"} offset=%d map_addr=0x%08x map_size=%d",
        ds.id, l4util_idstr(ds.manager), offset, map_addr, map_size);
  }
#endif

  ++_initialized;
  return 0;
}

/** Return amount of free kernel memory
 */
int l4dde_mm_kmem_avail(void)
{
  if (_initialized)
    return l4la_avail(&kpool);

  return 0;
}

/** Return begin and end of kmem regions */
int l4dde_mm_kmem_region(unsigned num, l4_addr_t *start, l4_addr_t *end)
{
#if 0
  if (num >= MM_KREGIONS)
    return -L4_EINVAL;

  *start = kregion[num].min;
  *end   = kregion[num].max;

  return 0;
#endif
  return -L4_EINVAL;
}
