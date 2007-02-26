/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/region.c
 * \brief  Region list handling.
 *
 * \date   06/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* L4RM includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/* VM size */
static l4_addr_t vm_start;  ///< VM start address
static l4_addr_t vm_end;    ///< VM end address

/**
 * pointer to region list 
 */
static l4rm_region_desc_t * head = NULL;

/**
 * region list lock 
 */
l4lock_t region_list_lock = L4LOCK_UNLOCKED_INITIALIZER;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Insert region into region list.
 * 
 * \param  region        new region descriptor 
 * \param  rp            free region in which the new region should be 
 *                       inserted
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_NOMEM  out of memory allocating region descriptor
 *
 * Insert region into region list at the place described by rp.
 */
/*****************************************************************************/ 
static int
__insert_region(l4rm_region_desc_t * region, l4rm_region_desc_t * rp)
{
  l4rm_region_desc_t * tmp;

  Assert(!FLAGS_EQUAL(region, rp));

  LOGdL(DEBUG_REGION_INSERT, "insert 0x%08x-0x%08x into 0x%08x-0x%08x",
        region->start, region->end, rp->start, rp->end);

  /* insert */
  if (rp->start == region->start) 
    {
      /* new region starts at beginning of the free region */
      if (rp->end == region->end)
	{
	  /* replace descriptor of free region with new descriptor */
	  if (rp == head)
	    {
	      region->prev = NULL;
	      region->next = rp->next;
	      if (rp->next)
		rp->next->prev = region;
	      head = region;
	    }
	  else
	    {
	      region->prev = rp->prev;
	      region->prev->next = region;
	      region->next = rp->next;
	      if (rp->next)
		rp->next->prev = region;
	    }

	  /* release region descriptor */
	  l4rm_region_desc_free(rp);
	}
      else
	{
	  /* shrink free region and insert new region */
	  rp->start = region->end + 1;
	  if (rp == head)
	    {
	      region->prev = NULL;
	      region->next = rp;
	      rp->prev = region;
	      head = region;
	    }
	  else
	    {
	      region->prev = rp->prev;
	      rp->prev->next = region;
	      region->next = rp;
	      rp->prev = region;
	    }
	}
    }
  else if (rp->end == region->end)
    {
      /* new region ends at the end of the free region, shrink free region */
      rp->end = region->start - 1;
      region->next = rp->next;
      region->prev = rp;
      if (region->next)
	region->next->prev = region;
      rp->next = region;
    }
  else
    {
      /* split free region */
      tmp =  l4rm_region_desc_alloc();
      if (tmp == NULL)
	/* out of memory */
	return -L4_ENOMEM;

      /* new free region behind new region */
      tmp->start = region->end + 1;
      tmp->end = rp->end;
      tmp->flags = rp->flags;
      tmp->prev = region;
      tmp->next = rp->next;
      if (tmp->next)
	tmp->next->prev = tmp;

      rp->end = region->start - 1;
      rp->next = region;
      
      region->prev = rp;
      region->next = tmp;
    } 

  /* done */
  return 0;
} 

/*****************************************************************************/
/**
 * \brief Change the flags of region.
 * 
 * \param  region        region descriptor
 * \param  new_flags     new flags for region
 *	
 * Change the flags of region to new_flags. Check if the previous/next
 * region then have the same flags and join.
 * 
 * \note After __modify_region returned the pointer region might be invalid
 *       because the region was joined with with the previous/next region.
 */
/*****************************************************************************/ 
static void
__modify_region(l4rm_region_desc_t * region, l4_uint32_t new_flags)
{
  l4rm_region_desc_t * tmp;

  /* set flags */
  region->flags = new_flags;

  if (IS_USED_REGION(region))
    /* we can't join used regions */
    return;

#if DEBUG_REGION_MODIFY
  if (region->prev)
    LOG_printf("prev   0x%08x-0x%08x, flags 0x%08x\n",
	   region->prev->start, region->prev->end, region->prev->flags);
  LOG_printf("region 0x%08x-0x%08x, flags 0x%08x\n",
	 region->start, region->end, region->flags);
  if (region->next)
    LOG_printf("next   0x%08x-0x%08x, flags 0x%08x\n",
	   region->next->start, region->next->end, region->next->flags);
#endif

  /* check previous/next region */
  if (region->prev && (FLAGS_EQUAL(region->prev, region)))
    {
      /* join with previous region */
      tmp = region->prev;
      tmp->end = region->end;
      tmp->next = region->next;
      if (tmp->next)
	tmp->next->prev = tmp;

      /* release region descriptor */
      l4rm_region_desc_free(region);

      region = tmp;
    }

  if (region->next && (FLAGS_EQUAL(region, region->next)))
    {
      /* join with next region */
      tmp = region->next;
      tmp->start = region->start;
      if (region == head)
	{
	  tmp->prev = NULL;
	  head = tmp;
	}
      else
	{
	  tmp->prev = region->prev;
	  tmp->prev->next = tmp;
	}

      /* release region descriptor */
      l4rm_region_desc_free(region);
    }

  /* done */
}

/*****************************************************************************/
/**
 * \brief  Find free region
 * 
 * \param  size          Size of free region
 * \param  area          Requested area id
 * \param  align         Alignment of start address
 * \retval region        Region descriptor of free region
 * \retval addr          Start address of free region 
 *                       (not necessarily equal to region->start, depends on 
 *                        requested alignment)
 *	
 * \return 0 on success, 
 *         error code (-#L4_ENOMAP) if no suitabele region found
 */
/*****************************************************************************/ 
static int
__find_free_region(l4_size_t size, l4_uint32_t area, int align, 
                   l4rm_region_desc_t ** region, l4_addr_t * addr)
{
  l4rm_region_desc_t * rp = head;
  l4_size_t a_size = 1UL << align;
  l4_addr_t a_addr, offs;

  /* search region */
  while (rp)
    {
      if (IS_FREE_REGION(rp) && (REGION_AREA(rp) == area))
	{
          a_addr = (rp->start + a_size - 1) & ~(a_size - 1);
          offs = a_addr - rp->start;
          if ((rp->end - rp->start + 1) >= (size + offs))
            {
              /* found suitable region */
              *region = rp;
              *addr = a_addr;
              return 0;
            }
        }
      rp = rp->next;
    }

  /* nothing found */
  return -L4_ENOMAP;
}

/*****************************************************************************/
/**
 * \brief  Search region
 * 
 * \param  addr          Address of region
 * \param  size          Size of region
 * \param  area          Area id
 * \retval region        Region descriptor
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invlaid addr / size argument
 *         - -#L4_EUSED   region already used
 */
/*****************************************************************************/ 
static int
__find_region(l4_addr_t addr, l4_size_t size, l4_uint32_t area,
              l4rm_region_desc_t ** region)
{
  l4rm_region_desc_t * rp = head;

  LOGdL(DEBUG_REGION_FIND, "addr 0x%08x, size %u, area 0x%05x", 
        addr, size, area);

  /* sanity checks */
  if ((addr < vm_start) || (addr > vm_end) || ((addr + size) > vm_end))
    return -L4_EINVAL;

  /* find region descriptor the address fits into */
  while (rp && (rp->end < addr))
    rp = rp->next;
  Assert(rp != NULL);

  LOGdL(DEBUG_REGION_FIND, "found area 0x%08x-0x%08x, flags 0x%08x",
        rp->start, rp->end, rp->flags);

  /* valid area? */
  if (IS_USED_REGION(rp) || (REGION_AREA(rp) != area) || 
      (rp->end < (addr + size - 1)))
    return -L4_EUSED;

  *region = rp;

  /* done */
  return 0;
}

/*****************************************************************************
 * L4RM internal library functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Setup region table.
 * 
 * \retval 0 on success, -1 if initialization failed.
 */
/*****************************************************************************/ 
int
l4rm_init_regions(void)
{
  int ret;

  /* init region descriptor allocation */
  ret = l4rm_region_alloc_init();
  if (ret < 0)
    return -1;

  /* setup first region which describes the whole user address space */
  vm_start = l4rm_get_vm_start();
  vm_end = l4rm_get_vm_end();

  head = l4rm_region_desc_alloc();
  if (head == NULL)
    return -1;

  head->start = vm_start;
  head->end = vm_end;
  head->prev = NULL;
  head->next = NULL;
  SET_REGION_FREE(head);
  SET_AREA(head, L4RM_DEFAULT_REGION_AREA);

  LOGdL(DEBUG_REGION_INIT, "L4RM: vm 0x%08x-0x%08x", 
        head->start, head->end + 1);

  /* done */
  return 0;
}
  
/*****************************************************************************/
/**
 * \brief  Set new region
 * 
 * \param  region        Descriptor of new region, it must contain valid 
 *                       entries for the dataspace / pagefault handler and
 *                       region flags. 
 * \param  addr          region start address 
 *                       (#L4RM_ADDR_FIND ... find suitable region)
 * \param  size          region size
 * \param  area          area id
 * \param  flags         flags:
 *                       - #L4RM_LOG2_ALIGNED  find a 2^(log2(size) + 1)
 *                                             aligned region
 *                       - #L4RM_SUPERPAGE_ALIGNED find a 
 *                                             superpage aligned region
 *                       - #L4RM_LOG2_ALLOC    allocate the whole 
 *                                             2^(log2(size) + 1) sized area
 *                       - #L4RM_MODIFY_DIRECT add new region without locking
 *                                             the region list and calling the
 *                                             region mapper thread
 *                       - #L4RM_TREE_INSERT   insert new region into region 
 *                                             tree
 * 	
 * \return 0 on success (the region descriptor contains the valid start / end
 *         entries), error code otherwise:
 *         - -#L4_ENOMAP    no suitable map area found
 *         - -#L4_EINVAL    invalid address / size argument
 *         - -#L4_EUSED     address area already used
 *         - -#L4_NOMEM     out of memory
 */
/*****************************************************************************/ 
int
l4rm_new_region(l4rm_region_desc_t * region, l4_addr_t addr, l4_size_t size, 
                l4_uint32_t area, l4_uint32_t flags)
{
  int ret, align;
  l4rm_region_desc_t * rp;
  l4_addr_t map_addr;

  LOGdL(DEBUG_REGION_NEW, "addr 0x%08x, size %u, area 0x%05x, flags 0x%08x",
        addr, size, area, flags);

  /* align size */
  size = l4_round_page(size);

  /* find / check region */
  if (addr == L4RM_ADDR_FIND)
    {
      align = L4_LOG2_PAGESIZE;
      if (flags & L4RM_LOG2_ALIGNED)
        {
          /* calculate alignment */
          align = l4util_log2(size);
          if (size > (1UL << align))
            align++;

          if (flags & L4RM_LOG2_ALLOC)
            size = 1UL << align;
        }

      if ((flags & L4RM_SUPERPAGE_ALIGNED) && (align < L4_LOG2_SUPERPAGESIZE))
	align = L4_LOG2_SUPERPAGESIZE;

      /* find free region */
      ret = __find_free_region(size, area, align, &rp, &map_addr);
      if (ret < 0)
        {
          LOGdL(DEBUG_REGION_NEW, "no suitable region found");
          return ret;
        }
    }
  else
    {
      /* align address */
      map_addr = l4_trunc_page(addr);

      /* check region */
      ret = __find_region(map_addr, size, area, &rp);
      if (ret < 0)
        {
          LOGdL(DEBUG_REGION_NEW, "invalid region (%s)", l4env_errstr(ret));
          return ret;
        }
    }

  LOGd(DEBUG_REGION_NEW, "using addr 0x%08x, size %u", map_addr, size);

  /* setup region descriptor */
  region->start = map_addr;
  region->end = map_addr + size - 1;

  /* create area id if requested */
  if (flags & L4RM_SET_AREA)
    area = map_addr >> L4_PAGESHIFT;
  SET_AREA(region, area);

  LOGd(DEBUG_REGION_NEW, 
       "\n create new region  0x%08x-0x%08x, flags 0x%08x\n" \
       " in existing region 0x%08x-0x%08x, flags 0x%08x",
       region->start, region->end, region->flags, 
       rp->start, rp->end, rp->flags);

  /* insert area in region list */
  ret = __insert_region(region, rp);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, "insert new region failed: %s (%d)", 
            l4env_errstr(ret), ret);
      return ret;
    }

  if (flags & L4RM_TREE_INSERT)
    {
      /* insert region in region tree */
      ret = l4rm_tree_insert_region(region, flags);
      if (ret < 0)
        {
          if (ret == -L4_EEXISTS)
            /* this should never happen */
            Panic("corrupted region list or AVL tree!");
          
          l4rm_free_region(region);
          return ret;
        }
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Mark region free.
 * 
 * \param  region        region descriptor
 */
/*****************************************************************************/ 
void
l4rm_free_region(l4rm_region_desc_t * region)
{
  /* sanity checks */
  if (!IS_USED_REGION(region))
    return;

  /* mark region free */
  SET_REGION_FREE(region);

  /* modify region list */
  return __modify_region(region, region->flags);
}

/*****************************************************************************/
/**
 * \brief Reset the area id.
 * 
 * \param  area          area id
 *
 * Reset the area id of all regions which belong to area \a area to the 
 * default area id.
 */
/*****************************************************************************/ 
void 
l4rm_reset_area(l4_uint32_t area)
{
  l4rm_region_desc_t * rp;
  int done;

  /* reset */
  done = 0;
  do
    {
      rp = head;

      while (rp && (REGION_AREA(rp) != area))
	rp = rp->next;

      if (!rp)
	/* done */
	done = 1;
      else
	{
	  /* reset area id */
	  SET_AREA(rp, L4RM_DEFAULT_REGION_AREA);
	  __modify_region(rp, rp->flags);
	}

      /* rp might be invalid after l4rm_modify_region, start from the 
       * list head again :( */
    }
  while (!done);
}  

/*****************************************************************************/
/**
 * \brief  Find region which given address belongs to
 * 
 * \param  addr          VM address
 *	
 * \return pointer to region NULL if not found
 */
/*****************************************************************************/ 
l4rm_region_desc_t *
l4rm_find_region(l4_addr_t addr)
{
  l4rm_region_desc_t * rp = head;

  /* Argh: we must manually search the given address in the region list, 
   *       reserved areas are not inserted into the region tree
   */

  /* sanity checks */
  if ((addr < vm_start) || (addr > vm_end))
    /* invalid address region */
    return NULL;

  LOGdL(DEBUG_REGION_FIND, "L4RM: find <0x%08x-0x%08x>", rp->start, rp->end);

  /* find region descriptor the address fits into */
  while (rp && (rp->end < addr))
    rp = rp->next;
  Assert(rp != NULL);

#if DEBUG_REGION_FIND
  LOG_printf("  found <0x%08x-0x%08x>\n", rp->start, rp->end);
#endif

  /* found */
  return rp;
}

/*****************************************************************************/
/**
 * \brief  Unmap region
 * 
 * \param  region        Region descriptor
 */
/*****************************************************************************/ 
void
l4rm_unmap_region(l4rm_region_desc_t * region)
{
  l4_addr_t addr = region->start;
  l4_size_t size = region->end - region->start + 1;
  int addr_align, log2_size, fpage_size;

  /* unmap pages
   * we try to use as few l4_fpage_unmap calls as possible to minimize the 
   * detach overhead for large regions.
   */
  LOGdL(DEBUG_REGION_UNMAP, "unmap addr 0x%08x, size %u", addr, size);

  while (size > 0)
    {
      LOGd(DEBUG_REGION_UNMAP,"0x%08x, size %u (0x%08x)",
           addr, size, size);

      /* calculate the largest fpage we can unmap at address addr, 
       * it depends on the alignment of addr and the size */
      addr_align = (addr == 0) ? 32 : l4util_bsf(addr);
      log2_size = l4util_log2(size);
      fpage_size = (addr_align < log2_size) ? addr_align : log2_size;

      LOGd(DEBUG_REGION_UNMAP, "addr %d, size %d, log2 %d",
           addr_align, log2_size, fpage_size);

      /* unmap page */
      l4_fpage_unmap(l4_fpage(addr, fpage_size, 0, 0),
		     L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

      addr += (1UL << fpage_size);
      size -= (1UL << fpage_size);
    }
}

/*****************************************************************************/
/**
 * \brief DEBUG: show region list
 */
/*****************************************************************************/ 
void
l4rm_show_region_list(void)
{
  l4rm_region_desc_t * rp = head;

  LOG_printf("region list:\n");
  while (rp)
    {
      LOG_printf("  area 0x%05x: 0x%08x - 0x%08x [%7dKiB]: ",
             REGION_AREA(rp), rp->start, rp->end, 
             (rp->end - rp->start + 1) >> 10);
      switch (REGION_TYPE(rp))
        {
        case REGION_FREE:
          if (REGION_AREA(rp) == L4RM_DEFAULT_REGION_AREA)
            LOG_printf("free\n");
          else
            LOG_printf("reserved\n");
          break;
        case REGION_DATASPACE:
          LOG_printf("ds %d at "l4util_idfmt"\n", rp->data.ds.ds.id,
                 l4util_idstr(rp->data.ds.ds.manager));
          break;
        case REGION_PAGER:
          LOG_printf("pager "l4util_idfmt"\n", 
	      l4util_idstr(rp->data.pager.pager));
          break;
        case REGION_EXCEPTION:
          LOG_printf("exception\n");
          break;
        case REGION_BLOCKED:
          LOG_printf("blocked\n");
          break;
        default:
          LOG_printf("unknown\n");
        }

      rp = rp->next;
    }
  LOG_printf("\n");
}
