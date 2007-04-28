/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/memmap.c
 * \brief  DMphys low level memory map
 *
 * \date   02/05/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <string.h>

/* L4/L4Env includes */
#include <l4/crtx/crt0.h>
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/sys/memdesc.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/slab/slab.h>
#include <l4/rmgr/librmgr.h>

/* DMphys includes */
#include "__memmap.h"
#include "__internal_alloc.h"
#include "__kinfo.h"
#include "__sigma0.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * memmap descriptor slab cache
 */
l4slab_cache_t memmap_cache;

/**
 * memmap descriptor slab cache name
 */
static char * memmap_cache_name = "memmap";

/**
 * low level memory map
 */
static memmap_t * mem = NULL;

/* phys memory available */
static l4_addr_t phys_mem_low;
static l4_addr_t phys_mem_high;

/* memory area to be used to assign memory pools */
static l4_addr_t mem_low;
static l4_addr_t mem_high;

/**
 * memory pool configurations
 */
static pool_cfg_t cfg[DMPHYS_NUM_POOLS];

/* pool names */
static char * default_pool_name = "Default memory pool";
#ifdef ARCH_x86
static char * isa_dma_pool_name = "ISA DMA memory pool";
#endif

/*****************************************************************************
 *** external symbols
 *****************************************************************************/

extern char _stext;  ///< DMphys binary start
extern char _end;    ///< DMphys binary end

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate memmap descriptor
 *
 * \return Pointer to memmap descriptor, NULL if allocation failed
 */
/*****************************************************************************/
static inline memmap_t *
__alloc_memmap_desc(void)
{
  memmap_t * mp = (memmap_t *)l4slab_alloc(&memmap_cache);

  if (mp == NULL)
    {
      Panic("DMphys: memory map descriptor allocation failed!");
      return NULL;
    }

  /* setup memmap descriptor */
  mp->pagesize = 0;
  mp->pool = MEMMAP_INVALID_POOL;
  mp->flags = MEMMAP_UNUSED;
  mp->prev = NULL;
  mp->next = NULL;

  return mp;
}

/*****************************************************************************/
/**
 * \brief  Release memmap descriptor
 *
 * \param  desc          Memmap descriptor
 */
/*****************************************************************************/
static inline void
__release_memmap_desc(memmap_t * desc)
{
  l4slab_free(&memmap_cache, desc);
}

/*****************************************************************************/
/**
 * \brief  Find memory area which contains address
 *
 * \param  addr          Address
 *
 * \return Pointer to memory area, NULL if address points beyond phys_mem_high
 */
/*****************************************************************************/
static inline memmap_t *
__find_addr(l4_addr_t addr)
{
  memmap_t * mp = mem;

  if (addr >= phys_mem_high)
    return NULL;

  /* find memmap entry which contains area */
  while ((mp != NULL) && ((mp->addr + mp->size) <= addr))
    mp = mp->next;

  return mp;
}

/*****************************************************************************/
/**
 * \brief  Cut area at end
 *
 * \param  area          Memmap area
 * \param  size          Size of area at end
 *
 * \return 0 on success, -1 if invalid size or descriptor allocation failed
 */
/*****************************************************************************/
static int
__cut_end(memmap_t * area, l4_size_t size)
{
  memmap_t * mp;

  if (size >= area->size)
    return -1;

  mp = __alloc_memmap_desc();
  if (mp == NULL)
    return -1;

  /* setup new area */
  mp->addr = area->addr + (area->size - size);
  mp->size = size;
  mp->pagesize = area->pagesize;
  mp->pool = area->pool;
  mp->flags = area->flags;

  area->size -= size;

  /* add new area into memory map */
  mp->prev = area;
  mp->next = area->next;
  if (mp->next != NULL)
    mp->next->prev = mp;
  area->next = mp;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Cut area at beginning
 *
 * \param  area          Memmap area
 * \param  size          Size of area at beginning
 *
 * \return 0 on success, -1 if invalid size or descriptor allocation failed
 */
/*****************************************************************************/
static int
__cut_begin(memmap_t * area, l4_size_t size)
{
  memmap_t * mp;

  if (size >= area->size)
    return -1;

  mp = __alloc_memmap_desc();
  if (mp == NULL)
    return -1;

  /* setup new area */
  mp->addr = area->addr;
  mp->size = size;
  mp->pagesize = area->pagesize;
  mp->pool = area->pool;
  mp->flags = area->flags;

  area->addr += size;
  area->size -= size;

  /* add new area into memory map */
  mp->next = area;
  mp->prev = area->prev;
  if (mp->prev != NULL)
    mp->prev->next = mp;
  area->prev = mp;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Cut area at beginning and set new area attributes
 *
 * \param  area          Memory area
 * \param  new           Size of area at beginning
 * \param  pool          Memory pool
 * \param  pagesize      Pagesize
 * \param  flags         Flags
 *
 * \return 0 on success, -1 if invalid size or descriptor allocation failed
 */
/*****************************************************************************/
static int
__mark_begin(memmap_t * area, l4_size_t size, l4_uint8_t pool,
	     l4_uint8_t pagesize, l4_uint16_t flags)
{
  memmap_t * mp;

#if DEBUG_MEMMAP_MAP
  LOG_printf("    area 0x%08lx-0x%08lx, pool %u, ps %u, flags 0x%04x\n",
         area->addr, area->addr + size, pool, pagesize, flags);
#endif

  if (size == area->size)
    {
      /* do not split, just set new attributes */
      area->pool = pool;
      area->pagesize = pagesize;
      area->flags = flags;
    }
  else
    {
      /* cut area at beginning */
      if (__cut_begin(area, size) < 0)
	return -1;

      /* set new area attributes */
      mp = area->prev;
      Assert(mp);
      mp->pool = pool;
      mp->pagesize = pagesize;
      mp->flags = flags;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Map memory region
 *
 * \param  low           Memory region start address
 * \param  high          Memory region size
 * \param  size          Amount to map
 * \param  pool          Assign to pool
 * \param  use_4M_page   Use 4MB-pages to map memory
 * \param  force         Force map (fail if mapping denied)
 *
 * \return 0 on success, -1 if something went wrong
 */
/*****************************************************************************/
static int
__map(l4_addr_t low, l4_addr_t high, l4_size_t size, int pool,
      int use_4M_pages, int force)
{
  memmap_t * mp;
  l4_addr_t addr,area_start;
  l4_size_t s,need,area_size;
  int last_mapped,last_4M,ret;

  addr = low;
  if (size == -1)
    need = mem_high - mem_low;
  else
    need = size;

  LOGdL(DEBUG_MEMMAP_MAP, "pool %d range 0x%08lx-0x%08lx, need 0x%08x",
        pool, low, high, need);

  /* find area which contains memory range start address */
  mp = __find_addr(addr);
  if (mp == NULL)
    return -1;

  /* make sure that the search starts at a area boundary */
  if (addr > mp->addr)
    {
      if (MEMMAP_IS_UNUSED(mp))
	{
	  /* split area at beginning */
	  if (__cut_begin(mp, addr - mp->addr) < 0)
	    return -1;
	}
      else
	{
	  /* start area already used or reseved, skip to next area */
	  addr = mp->addr + mp->size;
	  mp = mp->next;
	}
    }

  while ((mp != NULL) && (need > 0) && (addr < high))
    {
      /* skip reserved / already used areas */
      while ((mp != NULL) && (mp->addr < high) && !MEMMAP_IS_UNUSED(mp))
	mp = mp->next;

      if ((mp == NULL) || (mp->addr >= high))
	/* done */
	break;

#if DEBUG_MEMMAP_MAP
      LOG_printf(" area 0x%08lx-0x%08lx\n", mp->addr, mp->addr + mp->size);
#endif

      addr = mp->addr;

      s = mp->size;
      if ((mp->addr + mp->size) > high)
	s = high - mp->addr;

#if DEBUG_MEMMAP_MAP
      LOG_printf(" trying to map 0x%08lx-0x%08lx\n", addr, addr + s);
#endif

      area_start = addr;
      area_size = 0;
      last_mapped = 0;
      last_4M = 0;

      while ((s > 0) && (need > 0))
	{
	  /* try to map 4M page */
	  if (use_4M_pages && (s >= L4_SUPERPAGESIZE) &&
	      (need >= L4_SUPERPAGESIZE) && ((addr & ~L4_SUPERPAGEMASK) == 0))
	    {
	      if (dmphys_sigma0_map_4Mpage(addr) == 0)
		{
		  /* got 4M page */
		  if (!last_mapped || !last_4M)
		    {
		      /* last page was either denied or no 4MB-page, mark
		       * area in memory map */
		      if (area_size > 0)
			{
			  if (!last_mapped)
			    ret = __mark_begin(mp, area_size,
					       MEMMAP_INVALID_POOL, 0,
					       MEMMAP_DENIED);
			  else
			    ret = __mark_begin(mp, area_size, pool,
					       L4_LOG2_PAGESIZE,
					       MEMMAP_MAPPED);
			  if (ret < 0)
			    return -1;
			}

		      /* set start of new 4MB-page area */
		      area_start = addr;
		      area_size = L4_SUPERPAGESIZE;
		      last_mapped = 1;
		      last_4M = 1;
		    }
		  else
		    /* increase 4MB-page area */
		    area_size += L4_SUPERPAGESIZE;

		  addr += L4_SUPERPAGESIZE;
		  s -= L4_SUPERPAGESIZE;
		  need -= L4_SUPERPAGESIZE;

		  continue;
		}
	    }

	  /* no 4MB-page, try 4KB-page */
	  if (dmphys_sigma0_map_page(addr) == 0)
	    {
	      /* got 4KB-page */
	      if (!last_mapped || last_4M)
		{
		  /* last page was either denied or a 4MB-page, mark area in
		   * memory map */
		  if (area_size > 0)
		    {
		      if (!last_mapped)
			ret = __mark_begin(mp, area_size, MEMMAP_INVALID_POOL,
                                           0, MEMMAP_DENIED);
		      else
			ret = __mark_begin(mp, area_size, pool,
					   L4_LOG2_SUPERPAGESIZE,
					   MEMMAP_MAPPED);
		      if (ret < 0)
			return -1;
		    }

		  /* set start of new 4KB-page area */
		  area_start = addr;
		  area_size = L4_PAGESIZE;
		  last_mapped = 1;
		  last_4M = 0;
		}
	      else
		/* increase 4KB-page area */
		area_size += L4_PAGESIZE;

	      need -= L4_PAGESIZE;
	    }
	  else
	    {
	      /* did not get 4KB-page */
	      if (force)
		{
		  Panic("DMphys: failed to map reserved page at 0x%08lx!", addr);
		  return -1;
		}

	      if (last_mapped)
		{
		  /* last page was mapped, mark area in memory map */
		  if (area_size > 0)
		    {
		      if (last_4M)
			ret = __mark_begin(mp, area_size, pool,
					   L4_LOG2_SUPERPAGESIZE,
					   MEMMAP_MAPPED);
		      else
			ret = __mark_begin(mp, area_size, pool,
                                           L4_LOG2_PAGESIZE,
					   MEMMAP_MAPPED);
		      if (ret < 0)
			return -1;
		    }

		  /* set start of new denied area */
		  area_start = addr;
		  area_size = L4_PAGESIZE;
		  last_mapped = 0;
		}
	      else
		/* increase denied area */
		area_size += L4_PAGESIZE;
	    }

	  addr += L4_PAGESIZE;
	  s -= L4_PAGESIZE;

	} /* while (s > 0) */

      /* mark last area in memory map */
      if (area_size > 0)
	{
	  if (!last_mapped)
	    ret = __mark_begin(mp, area_size, MEMMAP_INVALID_POOL, 0,
			       MEMMAP_DENIED);
	  else
	    {
	      if (last_4M)
		ret = __mark_begin(mp, area_size, pool, L4_LOG2_SUPERPAGESIZE,
				   MEMMAP_MAPPED);
	      else
		ret = __mark_begin(mp, area_size, pool, L4_LOG2_PAGESIZE,
				   MEMMAP_MAPPED);
	    }

	  if (ret < 0)
	    return -1;
	}

    } /* while ((mp != NULL) && (need > 0) && (addr < high)) */

  /* scanned memory range */
  if ((size != -1) && (need > 0))
    {
      rmgr_init();
      rmgr_dump_mem();
      Panic("DMphys: not enough memory found for pool %d", pool);
      return -1;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Allocate areas for memory pools, RMGR version
 *
 * \return 0 on success, -1 if allocation failed
 */
/*****************************************************************************/
static int
__allocate_rmgr(int use_4M_pages)
{
  int i;
  l4_addr_t addr;

  if (!rmgr_init())
    {
      Panic("DMphys: RMGR not found!");
      return -1;
    }

  for (i = DMPHYS_NUM_POOLS - 1; i >= 0; i--)
    {
      LOGdL(DEBUG_MEMMAP_MAP, "pool %d", i);

      if (cfg[i].size != 0)
	{
	  if (cfg[i].size == -1)
	    {
	      Panic("DMphys: size must be specified if used with RMGR (pool %d)!",
		    i);
	      return -1;
	    }

#if DEBUG_MEMMAP_MAP
	  LOG_printf(" size 0x%08x, range 0x%08lx-0x%08lx\n", cfg[i].size,
                 cfg[i].low, cfg[i].high);
#endif

	  /* request memory area at RMGR (aligned) */
	  addr = rmgr_reserve_mem(cfg[i].size, l4util_bsr(cfg[i].size), 0,
				  cfg[i].low, cfg[i].high);
	  if (addr == -1)
	    {
	      rmgr_dump_mem();
	      Panic("DMphys: failed to reserve memory at rmgr "
		    "(pool %d, size %u)!", i, cfg[i].size);
	      return -1;
	    }

#if DEBUG_MEMMAP_MAP
	  LOG_printf(" got area at 0x%08lx\n", addr);
#endif

	  /* map area */
	  if (__map(addr,addr + cfg[i].size, cfg[i].size, i,
                    use_4M_pages, 1) < 0)
	    {
	      Panic("DMphys: map region 0x%08lx-0x%08lx reserved at RMGR "
		    "failed!", addr, addr + cfg[i].size);
	      return -1;
	    }
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Allocate areas for memory pools, normal version
 *
 * \param  use_4M_pages  Use 4MB-pages to map pools
 *
 * \return 0 on success, -1 if allocation failed
 */
/*****************************************************************************/
static int
__allocate(int use_4M_pages)
{
  int i;

  /* first, setup all pools with a specified size */
  for (i = DMPHYS_NUM_POOLS - 1; i >= 0; i--)
    {
      if ((cfg[i].size != 0) && (cfg[i].size != -1))
	{
	  if (__map(cfg[i].low, cfg[i].high, cfg[i].size, i,
                    use_4M_pages, 0) < 0)
	    {
	      Panic("DMphys: allocation of pool %d failed!", i);
	      return -1;
	    }
	}
    }

  /* now setup the pools which want to have all available memory */
  for (i = DMPHYS_NUM_POOLS - 1; i >= 0; i--)
    {
      if (cfg[i].size == -1)
	{
	  if (__map(cfg[i].low, cfg[i].high, -1, i, use_4M_pages, 0) < 0)
	    {
	      Panic("DMphys: allocation of pool %d failed!", i);
	      return -1;
	    }
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Add memory to page pools
 *
 * \return 0 on success, -1 if something went wrong
 */
/*****************************************************************************/
static int
__setup_pools(void)
{
  memmap_t * mp;
  l4_addr_t addr;
  l4_size_t size;
  int pool;

  mp = mem;
  while (mp != NULL)
    {
      while ((mp != NULL) && !MEMMAP_IS_MAPPED(mp))
	mp = mp->next;

      if (mp != NULL)
	{
	  pool = mp->pool;
	  addr = mp->addr;
	  size = mp->size;
	  mp = mp->next;

	  while ((mp != NULL) && MEMMAP_IS_MAPPED(mp) && (mp->pool == pool))
	    {
	      if (addr + size != mp->addr)
		break;
	      ASSERT((addr + size) == mp->addr);
	      size += mp->size;
	      mp = mp->next;
	    }

	  LOGdL(DEBUG_MEMMAP_POOLS, "pool %d, add 0x%08lx-0x%08lx",
                pool, addr, addr + size);

	  /* add memory region to pool */
	  if (dmphys_pages_add_free_area(pool, addr, size) < 0)
	    {
	      Panic("DMphys: add memory area 0x%08lx-0x%08lx to pool %d failed!",
		    addr, addr + size, pool);
	      return -1;
	    }
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Reserve memory area
 *
 * \param  addr          Memory area address
 * \param  size          Memory area size
 *
 * \return 0 on success, -1 if invalid area or area already used
 */
/*****************************************************************************/
static int
__reserve(l4_addr_t addr, l4_size_t size)
{
  memmap_t * mp, * tmp;
  l4_size_t got,need;

  LOGdL(DEBUG_MEMMAP_RESERVE, "reserve 0x%08lx-0x%08lx", addr, addr + size);

  /* find memmap area which contains addr */
  mp = __find_addr(addr);
  if (mp == NULL)
    return -1;

  /* make sure that the search starts at a area boundary */
  got = 0;
  if (addr > mp->addr)
    {
      if (MEMMAP_IS_RESERVED(mp))
	{
	  got = mp->size - (addr - mp->addr);
	  mp = mp->next;
	}
      else
	{
	  if (MEMMAP_IS_MAPPED(mp))
	    /* memory area already assigned to a memory pool, fail */
	    return -1;

	  /* split area */
	  if (__cut_begin(mp, addr - mp->addr) < 0)
	    return -1;
	}
    }

  /* reserve area */
  while (got < size)
    {
      need = size - got;

#if DEBUG_MEMMAP_RESERVE
      LOG_printf(" area 0x%08lx-0x%08lx, flags 0x%04x, need 0x%08x\n",
             mp->addr, mp->addr + mp->size, mp->flags, need);
#endif

      if (MEMMAP_IS_RESERVED(mp))
	got += mp->size;
      else
	{
	  if (MEMMAP_IS_MAPPED(mp))
	    /* memory area already assigned to a pool, fail */
	    return -1;

	  if (mp->size > need)
	    {
	      /* split area */
	      if (__cut_end(mp, mp->size - need) < 0)
		return -1;
	    }

	  /* set area reserved */
	  mp->flags = MEMMAP_RESERVED;
	  mp->pool = MEMMAP_INVALID_POOL;
	  mp->pagesize = 0;

	  got += mp->size;
	}

      /* if we reserve a non-contiguous memory area, we will get neighboring
       * areas which are reserved. Check that and join */
      if ((mp->prev) && MEMMAP_IS_RESERVED(mp->prev))
	{
	  /* join */
	  tmp = mp->prev;

	  mp->addr = tmp->addr;
	  mp->size += tmp->size;

	  mp->prev = tmp->prev;
	  if (mp->prev != NULL)
	    mp->prev->next = mp;
	  else
	    mem = mp;

	  __release_memmap_desc(tmp);
	}

      mp = mp->next;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Setup reserved memory areas
 */
/*****************************************************************************/
static void
__setup_reserved(void)
{
  l4_addr_t addr;
  l4_size_t size;

  /* BIOS page */
  addr = 0;
  size = L4_PAGESIZE;
  if (__reserve(addr, size) < 0)
    Panic("DMphys: reserve BIOS page at 0 failed!");

  /* lower I/O area */
  addr = 0x000A0000;
  size = 0x00060000;
  if (__reserve(addr, size) < 0)
    Panic("DMphys: reserve I/O area at 0x%08lx-0x%08lx failed!",
          addr, addr + size);

  /* DMphys binary */
  addr = l4_trunc_page((l4_addr_t)&_stext);
  size = l4_round_page((l4_addr_t)&_end) - addr;
  if (__reserve(addr, size) < 0)
    Panic("DMphys: reserve DMphys binary at 0x%08lx-0x%08lx failed!",
          addr, addr + size);

  /* RMGR trampoline page */
  addr = l4_trunc_page(crt0_tramppage);
  size = l4_round_page(crt0_tramppage + L4_PAGESIZE) - addr;
  if (__reserve(addr, size) < 0)
    Panic("DMphys: reserve RMGR trampoline page at 0x%08lx-0x%08lx failed!",
          addr, addr + size);

}

/*****************************************************************************/
/**
 * \brief  Check 4MB-page assumption (see dm_phys/doc/4MB-pages.dox)
 *
 * \param  use_4M_pages  Use 4MB-pages
 */
/*****************************************************************************/
static void
__check_assumptions(int use_4M_pages)
{
  memmap_t * mp = mem;
  l4_addr_t next_4M_addr,end_addr;

  if (!use_4M_pages)
    /* nothing to check */
    return;

  /* check if all contiguous 4MB-areas are mapped as 4MB-pages */
  while (mp != NULL)
    {
      if (MEMMAP_IS_MAPPED(mp))
	{
	  /* does area contain 4MB aligned/sized area? */
	  next_4M_addr =
	    (mp->addr + DMPHYS_SUPERPAGESIZE - 1) & ~(DMPHYS_SUPERPAGESIZE - 1);
	  if ((next_4M_addr + DMPHYS_SUPERPAGESIZE) <= (mp->addr + mp->size))
	    {
	      if (mp->pagesize != DMPHYS_LOG2_SUPERPAGESIZE)
		{
		  end_addr = (mp->addr + mp->size) & ~(DMPHYS_SUPERPAGESIZE - 1);
		  LOG_printf("DMphys: area 0x%08lx-0x%08lx not mapped using "
                         "4MB-pages!\n", next_4M_addr, end_addr);
		  LOG_printf("Warning: 4MB-page assumptions not met!\n");
		}
	    }
	}

      mp = mp->next;
    }
}

static
void
__insert_ram(memmap_t *mp)
{
  if (!mem)
    {
      mem = mp;
      return;
    }

  memmap_t **m = &mem;
  while (*m && (*m)->addr < mp->addr)
    m = &((*m)->next);

  mp->next = *m;
  if (*m)
    mp->prev = (*m)->prev;

  *m = mp;
}

static
void
__setup_free_regions(void)
{
  /* set memory range to be used to assigne memory pools */
  mem_low = (phys_mem_low > DMPHYS_MEM_LOW) ? phys_mem_low : DMPHYS_MEM_LOW;
  mem_high = phys_mem_high;
  mem = 0;
  memmap_t *mp = 0;

  l4_kernel_info_t *kip = dmphys_sigma0_kinfo();
  l4_kernel_info_mem_desc_t *md = l4_kernel_info_get_mem_descs(kip);
  unsigned n = l4_kernel_info_get_num_mem_descs(kip);
  unsigned i;
  for (i = 0; i < n; ++md, ++i)
    {
      if (   l4_kernel_info_get_mem_desc_type(md) == l4_mem_type_conventional
	&& !l4_kernel_info_get_mem_desc_is_virtual(md))
	{
	  unsigned long start = l4_kernel_info_get_mem_desc_start(md);
	  unsigned long end   = l4_kernel_info_get_mem_desc_end(md);
	  mp = __alloc_memmap_desc();
	  if (mp == NULL)
	    Panic("DMphys: cannot allocate memmap descriptor!");;

	  mp->addr = start;
	  mp->size = end-start + 1;

	  __insert_ram(mp);
	}
    }

  /* if we skip low memory by default, mark it reserved */
  if (mem_low > phys_mem_low)
    {
      if (__reserve(phys_mem_low, mem_low - phys_mem_low) < 0)
	Panic("DMphys: reserve unused low memory failed!");
    }
}

/*****************************************************************************
 *** DMphys internal API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Initialize memmap handling
 *
 * \return 0 on success, -1 if something went wrong
 */
/*****************************************************************************/
int
dmphys_memmap_init(void)
{
  int i;
  pool_cfg_t * pool;

  /* initialize memmap descriptor slab cache */
  if (l4slab_cache_init(&memmap_cache, sizeof(memmap_t), 0,
			dmphys_internal_alloc_grow,
			dmphys_internal_alloc_release) < 0)
    return -1;

  l4slab_set_data(&memmap_cache, memmap_cache_name);

  /* setup initial memory map */
  phys_mem_low = dmphys_kinfo_mem_low();
  phys_mem_high = dmphys_kinfo_mem_high();


  __setup_free_regions();

  /* setup other reserved memory areas */
  __setup_reserved();

  /* setup pool configuration */
  for (i = 0; i < DMPHYS_NUM_POOLS; i++)
    {
      cfg[i].size = 0;
      cfg[i].low = -1;
      cfg[i].high = -1;
      cfg[i].reserved = 0;
      cfg[i].name[0] = '\0';
    }

  /* setup default pool configuration */
  pool = &cfg[DMPHYS_MEM_DEFAULT_POOL];
  strcpy(pool->name,default_pool_name);
  pool->size = DMPHYS_MEM_DEFAULT_SIZE;
  pool->low = DMPHYS_MEM_DEFAULT_LOW;
  pool->high = DMPHYS_MEM_DEFAULT_HIGH;
  pool->reserved = DMPHYS_MEM_DEFAULT_RESERVED;

#ifdef ARCH_x86
  pool = &cfg[DMPHYS_MEM_ISA_DMA_POOL];
  strcpy(pool->name,isa_dma_pool_name);
  pool->low = DMPHYS_MEM_ISA_DMA_LOW;
  pool->high = DMPHYS_MEM_ISA_DMA_HIGH;
  pool->size = DMPHYS_MEM_ISA_DMA_SIZE;
  pool->reserved = DMPHYS_MEM_ISA_DMA_RESERVED;
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Setup memory pools
 *
 * \param  use_rmgr      Use RMGR to reserve memory areas
 * \param  use_4M_pages  Use 4MB-pages to map memory areas
 *
 * \return 0 on success, -1 if setup failed
 */
/*****************************************************************************/
int
dmphys_memmap_setup_pools(int use_rmgr, int use_4M_pages)
{
  int ret,i;

  /* adapt search ranges to phys. memory layout */
  for (i = 0; i < DMPHYS_NUM_POOLS; i++)
    {
      /* search range start address */
      if (cfg[i].low == -1)
	cfg[i].low = mem_low;

      if (cfg[i].low < mem_low)
	cfg[i].low = mem_low;

      if (cfg[i].low > mem_high)
	cfg[i].low = mem_high;

      /* search range end address */
      if (cfg[i].high == -1)
	cfg[i].high = mem_high;

      if (cfg[i].high < mem_low)
	cfg[i].high = mem_low;

      if (cfg[i].high > mem_high)
	cfg[i].high = mem_high;
    }

  /* init page pool descriptors */
  for (i = 0; i < DMPHYS_NUM_POOLS; i++)
    dmphys_pages_init_pool(i, cfg[i].reserved, cfg[i].name);

  /* allocate and map pool areas */
  if (use_rmgr)
    ret = __allocate_rmgr(use_4M_pages);
  else
    ret = __allocate(use_4M_pages);
  if (ret)
    return -1;

  /* check 4MB page assumptions */
  __check_assumptions(use_4M_pages);

  /* add memory to page pools */
  if (__setup_pools() < 0)
    return -1;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Mark memory area reserved
 *
 * \param  addr          Memory area address
 * \param  size          Memory area size
 *
 * \return 0 on success, -1 if reservation failed
 */
/*****************************************************************************/
int
dmphys_memmap_reserve(l4_addr_t addr, l4_size_t size)
{
  /* mark reserved */
  return __reserve(addr, size);
}

/*****************************************************************************/
/**
 * \brief  Set memory map search low address
 *
 * \param  low           Low address
 */
/*****************************************************************************/
void
dmphys_memmap_set_mem_low(l4_addr_t low)
{
  if (low < phys_mem_low)
    mem_low = phys_mem_low;

  if (low > phys_mem_high)
    mem_low = phys_mem_high;

  mem_low = low;
}

/*****************************************************************************/
/**
 * \brief  Set memory map search high address
 *
 * \param  high          High address
 */
/*****************************************************************************/
void
dmphys_memmap_set_mem_high(l4_addr_t high)
{
  if (high < phys_mem_low)
    mem_high = phys_mem_low;

  if (high > phys_mem_high)
    mem_high = phys_mem_high;

  mem_high = high;
}

/*****************************************************************************/
/**
 * \brief  Set pool configuration
 *
 * \param  pool          Pool number
 * \param  size          Pool area size
 * \param  low           Search memory range low address
 * \param  high          Search memory range high address
 * \param  name          Page pool name (optional)
 *
 * \return 0 on success, -1 if invalid pool number
 */
/*****************************************************************************/
int
dmphys_memmap_set_pool_config(int pool, l4_size_t size,
			      l4_addr_t low, l4_addr_t high,
			      const char * name)
{
  if ((pool < 0) || (pool >= DMPHYS_NUM_POOLS))
    return -1;

  /* set pool configuration */
  cfg[pool].size = size;
  if (low != 0)
    cfg[pool].low = low;
  if (high != 0)
    cfg[pool].high = high;
  if (name != 0)
    {
      strncpy(cfg[pool].name, name, DMPHYS_MEM_POOL_NAME_LEN);
      cfg[pool].name[DMPHYS_MEM_POOL_NAME_LEN - 1] = 0;
    }

  LOGdL(DEBUG_MEMMAP_POOLS, "pool %d: size 0x%08x, range 0x%08lx-0x%08lx",
        pool, size, low, high);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Check pagesize of memory area
 *
 * \param  addr          Memory area address
 * \param  size          Memory area size
 * \param  pagesize      Pagesize
 *
 * \return 1 if the area is mapped with the specified pagesize (or a larger
 *         pagesize) and has the correct allignment, 0 otherwise
 */
/*****************************************************************************/
int
dmphys_memmap_check_pagesize(l4_addr_t addr, l4_size_t size, int pagesize)
{
  l4_addr_t  align_mask;
  l4_size_t  got;
  memmap_t * mp;

  if ((pagesize > (L4_WHOLE_ADDRESS_SPACE - 1)) ||
      (pagesize < L4_LOG2_PAGESIZE))
    return 0;

  LOGdL(DEBUG_MEMMAP_PAGESIZE, "area 0x%08lx-0x%08lx, pagesize %d",
        addr, addr + size, pagesize);

  /* check address / size alignment */
  align_mask = (1UL << pagesize) - 1;

  if (((addr & align_mask) != 0) || ((size & align_mask) != 0))
    return 0;

#if DEBUG_MEMMAP_PAGESIZE
  LOG_printf(" addr / size alignment ok\n");
#endif

  /* find area in memory map */
  mp = __find_addr(addr);
  got = 0;
  while ((mp != NULL) && (got < size))
    {
#if DEBUG_MEMMAP_PAGESIZE
      LOG_printf(" area 0x%08lx-0x%08lx, pagesize %d, flags 0x%04x\n",
             mp->addr, mp->addr + mp->size, mp->pagesize, mp->flags);
#endif

      if (!MEMMAP_IS_MAPPED(mp))
	{
	  LOG_Error("DMphys: memory area 0x%08lx-0x%08lx not mapped!",
                    addr, addr + size);
	  return 0;
	}

      if (mp->pagesize < pagesize)
	return 0;

      got += (mp->size - (addr - mp->addr));
      addr += (mp->size - (addr - mp->addr));
      mp = mp->next;
    }

  if (got < size)
    {
      LOG_Error("DMphys: memory area 0x%08lx-0x%08lx not mapped!",
                addr, addr + size);
      return 0;
    }

  /* done */
  return 1;
}

/*****************************************************************************/
/**
 * \brief  DEBUG: dump memory map
 */
/*****************************************************************************/
void
dmphys_memmap_show(void)
{
  memmap_t * mp = mem;

  LOG_printf("DMphys memory map:\n");
  LOG_printf("  phys. memory 0x%08lx-0x%08lx (from L4 kernel info page)\n",
         phys_mem_low, phys_mem_high);
  LOG_printf("  using 0x%08lx-0x%08lx\n", mem_low, mem_high);
  LOG_printf("       Memory area      Pool  PS  Flags\n");
  while (mp != NULL)
    {
      if (MEMMAP_IS_MAPPED(mp))
	LOG_printf("  0x%08lx-0x%08lx  %2u   %2u  ",mp->addr, mp->addr + mp->size,
               mp->pool, mp->pagesize);
      else
	LOG_printf("  0x%08lx-0x%08lx  --   --  ", mp->addr, mp->addr + mp->size);

      if (mp->flags & MEMMAP_UNUSED)
	LOG_printf("UNUSED ");
      if (mp->flags & MEMMAP_MAPPED)
	LOG_printf("MAPPED ");
      if (mp->flags & MEMMAP_RESERVED)
	LOG_printf("RESERVED ");
      if (mp->flags & MEMMAP_DENIED)
	LOG_printf("DENIED");
      LOG_printf("\n");

      mp = mp->next;
    }
}
