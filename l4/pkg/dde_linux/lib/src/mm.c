/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/src/mm.c
 *
 * \brief	Memory Management
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
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
 * Requirements: (additionally to \ref pg_req)
 *
 * - OSKit lmm library
 *
 * Configuration:
 *
 * - setup #MM_KREGIONS to change number of kernel memory regions (default 1!)
 * - maximum size of pools is configured via parameters of l4dde_mm_init()
 */
/*****************************************************************************/

/* L4 */
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/names/libnames.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/slab.h>
#include <linux/vmalloc.h>

/* OSKit */
#include <oskit/lmm.h>

/* local */
#include "__config.h"
#include "__macros.h"
#include "internal.h"

/*****************************************************************************/
/** 
 * \name Module Variables
 * @{ */
/*****************************************************************************/
/** initialization flag */
static int _initialized = 0;

/* kernel memory - kmem */
/** kmem pool */
static lmm_t kpool = LMM_INITIALIZER;
/** kmem regions in pool */
static lmm_region_t kregion[MM_KREGIONS];
/** kmem region size */
static unsigned int kregion_size;
/** kmem region counter */
static unsigned int kcount = MM_KREGIONS - 1;
/** protect access to kmem pool */
static l4lock_t klock = L4LOCK_UNLOCKED_INITIALIZER;

/* virtual memory - vmem */
/** vmem pool */
static lmm_t vpool = LMM_INITIALIZER;
/** vmem region in pool */
static lmm_region_t vregion;
/** protect access to vmem pool */
static l4lock_t vlock = L4LOCK_UNLOCKED_INITIALIZER;

/** @} */
/*****************************************************************************/
/** Simulated LMM Callback on Memory Shortage (kmem)
 *
 * \param  size		size of memory chunk needed
 * \param  flags	memory type
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
 * \todo consider \p flags
 */
/*****************************************************************************/
static __inline__ int __more_kcore(l4_size_t size, l4_uint32_t flags)
{
  int error;
  l4_addr_t kaddr;

  l4_size_t tmp;
  l4dm_mem_addr_t dm_paddr;

  if (!(kcount) || (size > kregion_size))
    {
      ERROR("out of memory (kmem)");
      return -L4_ENOMEM;
    }

#if DEBUG_MALLOC
  DMSG("requesting %d bytes (kmem)\n", size);
#endif

  /* open and attach new dataspace */
  kaddr = (l4_addr_t) \
    l4dm_mem_allocate_named(kregion_size,
			    L4DM_CONTIGUOUS | L4DM_PINNED | L4RM_MAP,
			    "dde kmem");
  if (!kaddr)
    {
      ERROR("allocating kmem");
      return -L4_ENOMEM;
    }

  error = l4dm_mem_phys_addr((void *) kaddr, 1, &dm_paddr, 1, &tmp);
  if (error != 1)
    {
      if (error>1 || !error)
	Panic("Ouch, what's that?");
      ERROR("getting physical address (%d)", error);
      return error;
    }

#if DEBUG_MALLOC
  DMSG("adding %d Bytes (kmem) @ 0x%08x (phys. 0x%08x) region %d\n",
       size, kaddr, dm_paddr.addr, kcount);
#endif

  /* add new region */
  lmm_add_region(&kpool, &kregion[kcount--], (void *) kaddr, size, 0, 0);

  /* add free memory area to region/pool */
  lmm_add_free(&kpool, (void *) kaddr, size);

  /* address info */
  address_add_region(kaddr, dm_paddr.addr, size);

  return 0;
}

/*****************************************************************************/
/** Simulated LMM Callback on Memory Shortage (vmem)
 *
 * \param  size		size of memory chunk needed
 *
 * \return -L4_ENOMEM
 *
 * This always returns "Out of memory" as we leave mappings in vmem region to
 * l4rm and dataspace managers.
 */
/*****************************************************************************/
static __inline__ int __more_vcore(l4_size_t size)
{
  ERROR("out of memory (vmem)");
  return -L4_ENOMEM;
}

/*****************************************************************************/
/** kmem Allocation
 * \ingroup mod_mm
 *
 * \param  size		size of memory chunk
 * \param  gfp		flags
 *
 * This is Linux' %kmalloc().
 *
 * \return start address of allocated chunk; NULL on error
 *
 */
/*****************************************************************************/
void *kmalloc(size_t size, int gfp)
{
  lmm_flags_t lmm_flags = 0;
  l4_uint32_t *chunk;

  if (gfp & GFP_DMA)
    DMSG("Warning: No ISA DMA implemented.\n");
   
  /* mutex pool access */
  l4lock_lock(&klock);

  size += sizeof(size_t);
  while (!(chunk = lmm_alloc(&kpool, size, lmm_flags)))
    {
      if (__more_kcore(size, lmm_flags))
	{
#if DEBUG_MALLOC
	  DMSG("failed to allocate %d bytes (kmem)\n", size);
	  lmm_dump(&kpool);
	  lmm_stats(&kpool);
#endif
	  return NULL;
	}
    }
  *chunk = size;

#if DEBUG_MALLOC
  DMSG("allocated %d bytes @ %p (kmem)\n", *chunk, chunk);
#endif

  l4lock_unlock(&klock);

  return ++chunk;
}

/*****************************************************************************/
/** kmem Deallocation
 * \ingroup mod_mm
 *
 * \param  addr		start address of memory chunk to free
 *
 * This is Linux' %kfree().
 */
/*****************************************************************************/
void kfree(const void *addr)
{
  l4_uint32_t *chunk = (l4_uint32_t *) addr - 1;

  if (!addr)
    return;

  /* mutex pool access */
  l4lock_lock(&klock);

#if DEBUG_MALLOC
  DMSG("freeing %d bytes @ %p (kmem)\n", *chunk, chunk);
#endif

  lmm_free(&kpool, chunk, *chunk);

  l4lock_unlock(&klock);
}

/*****************************************************************************/
/** vmem Allocation
 * \ingroup mod_mm
 *
 * \param  size		size of memory chunk
 *
 * \return start address of allocated chunk; NULL on error
 *
 * This is Linux' %vmalloc().
 */
/*****************************************************************************/
void *vmalloc(unsigned long size)
{
  lmm_flags_t lmm_flags = 0;
  l4_uint32_t *chunk;

  /* mutex pool access */
  l4lock_lock(&vlock);

  size += sizeof(size_t);
  while (!(chunk = lmm_alloc(&vpool, size, lmm_flags)))
    {
      if (__more_vcore(size))
	{
#if DEBUG_MALLOC
	  DMSG("failed to allocate %ld bytes (vmem)\n", size);
	  lmm_dump(&vpool);
	  lmm_stats(&vpool);
#endif
	  return NULL;
	}
    }
  *chunk = size;

#if DEBUG_MALLOC
  DMSG("allocated %d bytes @ %p (vmem)\n", *chunk, chunk);
#endif

  l4lock_unlock(&vlock);

  return ++chunk;
}

/*****************************************************************************/
/** vmem Deallocation
 * \ingroup mod_mm
 *
 * \param  addr		start address of memory chunk to release
 *
 * This is Linux' %vfree().
 */
/*****************************************************************************/
void vfree(void *addr)
{
  l4_uint32_t *chunk = (l4_uint32_t *) addr - 1;

  if (!addr)
    return;

  /* mutex pool access */
  l4lock_lock(&vlock);

  lmm_free(&vpool, chunk, *chunk);

#if DEBUG_MALLOC
  DMSG("freed %d bytes @ %p (vmem)\n", *chunk, chunk);
#endif

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
#if DEBUG_MALLOC
  DMSG("size/regions = %d\n", kregion_size);
#endif

  kregion_size = (kregion_size + L4_PAGESIZE - 1) & L4_PAGEMASK;
#if DEBUG_MALLOC
  DMSG("  rsize (mod PAGESIZE) = %d\n", kregion_size);
#endif

  if (kregion_size * MM_KREGIONS < size)
    {
      kregion_size += L4_PAGESIZE;
#if DEBUG_MALLOC
      DMSG("  new rsize = %d\n", kregion_size);
#endif
    }

#if DEBUG_MALLOC
  DMSG("kregion_size = 0x%x regions = %d\n", kregion_size, MM_KREGIONS);
#endif

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
      ERROR("getting physical address (%d)", error);
      return error;
    }

#if DEBUG_MALLOC
  DMSG("adding %d Bytes (kmem) @ 0x%08x (phys. 0x%08x) region 0\n",
       kregion_size, *addr, dm_paddr.addr);
#endif

  /* add initial region 0 */
  lmm_add_region(&kpool, &kregion[0], (void *) *addr, kregion_size, 0, 0);

  /* add free memory area to region/pool */
  lmm_add_free(&kpool, (void *) *addr, kregion_size);

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

  /* add region to vmem pool */
  lmm_add_region(&vpool, &vregion, (void *) *addr, size, 0, 0);
  /* add free memory area */
  lmm_add_free(&vpool, (void *) *addr, size);

  *max = size;
  return 0;
}

/*****************************************************************************/
/** Initalize LMM pools and initial regions.
 * \ingroup mod_mm
 *
 * \param  max_vsize	max size of virtual memory allocations
 * \param  max_ksize	max size of kernel memory allocations
 *
 * \return 0 on success, negative error code otherwise
 *
 * Sizes are rounded up to multiples of page size.
 *
 * kmem pool is divided into MM_KREGIONS regions of same size. The total size
 * will be \c max_ksize (or greater because of rounding/page size). vmem pool
 * has only one region of \c max_vsize.
 */
/*****************************************************************************/
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
      ERROR("setting up vmem: %d (%s)", error, l4env_strerror(-error));
      return error;
    }

  /* setup kernel memory pool */
  if ((error=__setup_kmem(&max_ksize, &kaddr)))
    {
      ERROR("setting up kmem %d (%s)", error, l4env_strerror(-error));
      return error;
    }

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
  Msg("Using ...\n"
      "  %d %s at 0x%08x (vmem)\n"
      "  %d %s in %d regions (kmem)\n",
      max_vsize, vsize_str, vaddr, max_ksize, ksize_str, MM_KREGIONS);

#if DEBUG_MALLOC
  {
    int debug;
    l4dm_dataspace_t ds;
    l4_offs_t offset;
    l4_addr_t map_addr;
    l4_size_t map_size;

    debug = l4rm_lookup((void*)vaddr, &ds, &offset, &map_addr, &map_size);
    if (debug)
      Panic("l4rm_lookup failed (%d)", debug);
    DMSG("vmem: ds={%3u, "IdFmt"} offset=%d map_addr=0x%08x map_size=%d\n",
	 ds.id, IdStr(ds.manager), offset, map_addr, map_size);

    debug = l4rm_lookup((void*)kaddr, &ds, &offset, &map_addr, &map_size);
    if (debug)
      Panic("l4rm_lookup failed (%d)", debug);
    DMSG("kmem: ds={%3u, "IdFmt"} offset=%d map_addr=0x%08x map_size=%d\n",
	 ds.id, IdStr(ds.manager), offset, map_addr, map_size);
  }
#endif

  ++_initialized;
  return 0;
}
