/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/alloc.c
 * \brief  Memory allocation for region descriptor / AVL tree node slab caches
 *
 * \date   07/31/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/sys/ipc.h>
#include <l4/env/errno.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>

/* local includes */
#include <l4/l4rm/l4rm.h>
#include "__alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/// number of pages in heap 
#define NUM_PAGES  (L4RM_MAX_HEAP_SIZE / L4_PAGESIZE)

/// heap page table
static int pages[NUM_PAGES];

/// next heap page for allocation 
static int heap_next_page;

/// heap start address
static l4_addr_t heap_start;

///< heap page map address 
#define PAGE_MAP_ADDR(i) (heap_start + i * L4_PAGESIZE)

/// use L4 environment services (dataspace manager) to allocate pages 
static int use_l4env = 0;

/// Heap dataspace 
static l4dm_dataspace_t heap_ds;

/// Sigma0 id
static l4_threadid_t sigma0_id = L4_INVALID_ID;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate and map memory (Sigma0)
 * 
 * \param  page          Heap page index
 *	
 * \return pointer to heap page on success, NULL if allocation failed
 */
/*****************************************************************************/ 
static inline void *
__sigma0_allocate(int page)
{
  l4_addr_t addr = PAGE_MAP_ADDR(page);
  int error;
  l4_umword_t base;
  l4_fpage_t fp;
  l4_msgdope_t result;

#if DEBUG_ALLOC_PAGE
  INFO("L4RM heap (sigma0):\n");
  DMSG("  heap page %d at 0x%08x\n",page,addr);
#endif
  
  if (!pages[page])
    {
      /* call pager to allocate new page */
      error = l4_i386_ipc_call(sigma0_id,L4_IPC_SHORT_MSG,0xFFFFFFFC,0,
			       L4_IPC_MAPMSG(addr, L4_LOG2_PAGESIZE),
			       &base,&fp.fpage,L4_IPC_NEVER,&result);
      if (error)
	{
	  Panic("L4RM heap: error calling task pager (result 0x%08x)!",
		result.msgdope);
	  return NULL;
	}
      
      if (!l4_ipc_fpage_received(result))
	{
	  Panic("L4RM heap: page allocation failed (result 0x%08x)!",
		result.msgdope);
	  return NULL;
	}
      
#if DEBUG_ALLOC_PAGE
      DMSG("  got page at 0x%08x\n",fp.fp.page << L4_LOG2_PAGESIZE);
#endif
    }

  /* done */
  return (void *)addr;
}

/*****************************************************************************/
/**
 * \brief Allocate and map memory (L4Env)
 * 
 * \param  page          Heap page index
 *	
 * \return pointer to heap page on success, NULL if allocation failed
 */
/*****************************************************************************/ 
static inline void *
__l4env_allocate(int page)
{
  l4_addr_t addr = PAGE_MAP_ADDR(page);
  l4_size_t new_size = (addr - heap_start) + L4_PAGESIZE;
  l4_offs_t offs = addr - heap_start;
  l4_addr_t fpage_addr;
  l4_size_t fpage_size;
  int ret;
  
#if DEBUG_ALLOC_PAGE
  INFO("L4RM heap (L4Env):\n");
  DMSG("  heap page %d at 0x%08x\n",page,addr);
#endif
  
  if (!pages[page])
    {
      /* resize heap dataspace */
#if DEBUG_ALLOC_PAGE
      DMSG("  resize heap dataspace, new size 0x%08x\n",new_size);
#endif
      
      ret = l4dm_mem_resize(&heap_ds,new_size);
      if (ret < 0)
	{
	  Panic("L4RM heap: resize heap dataspace failed: %s (%d)!",
		l4env_errstr(ret),ret);
	  return NULL;
	}
      pages[page] = 1;

#if DEBUG_ALLOC_PAGE    
      l4dm_ds_show(&heap_ds);
#endif
    }
  
  /* map page */
#if DEBUG_ALLOC_PAGE
  DMSG("  map page, ds offset 0x%08x\n",offs);
#endif
  ret = l4dm_map_pages(&heap_ds,offs,L4_PAGESIZE,addr,L4_LOG2_PAGESIZE,
		       0,L4DM_RW,&fpage_addr,&fpage_size);
  if (ret < 0)
    {
      Panic("L4RM heap: map dataspace page failed: %s (%d)",
	    l4env_errstr(ret),ret);
      return NULL;
    }

  /* done */
  return (void *)addr;
}

/*****************************************************************************
 *** L4RM internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Initialize memory allocation.
 * 
 * \param  have_l4env    Use the L4 environment to allocate memory 
 *                       (dataspace manager)
 * \param  used          Used VM address range, do not use for internal data.
 * \param  num_used      Number of elements in \a used.
 *	
 * \return 0 on success, -1 if initalization failed.
 *
 * This function must be called before the first region descriptor / AVL tree 
 * node is allocated. Don't forget to call l4rm_heap_register() after 
 * the initialization of the region list / AVL tree to reserve the vm area 
 * used for the heap.
 */
/*****************************************************************************/ 
int
l4rm_heap_init(int have_l4env, 
	       l4rm_vm_range_t used[], 
	       int num_used)
{
  int i,found,ret;
  l4_addr_t addr, end, test_end, used_end;
  l4_size_t size;
  l4_threadid_t dsm_id;

  /* Find heap map address. The region allocator is not yet running, we must 
   * ensure manually that the area does not overlap any used vm ranges 
   * (the binary and rmgr trampoline page if started by rmgr / libloader and
   * environment info page if started by the loader). 
   */
  addr = l4rm_get_vm_start();
  test_end = l4rm_get_vm_end() - L4RM_MAX_HEAP_SIZE + 1;
  size = L4RM_MAX_HEAP_SIZE;

  if (l4rm_heap_start_addr == -1)
    {
      /* search for suitable map address */ 
#if DEBUG_ALLOC_INIT
      INFO("\n");
      DMSG("  testing vm range 0x%08x-0x%08x\n",addr,test_end);
      DMSG("  used areas:\n");
      for (i = 0; i < num_used; i++)
	DMSG("    0x%08x-0x%08x\n",used[i].addr,used[i].addr + used[i].size);
#endif
      
      found = 0;
      while ((addr < test_end) && !found)
	{
	  end = addr + size;
	  
	  found = 1;
	  for (i = 0; i < num_used; i++)
	    {
	      used_end = used[i].addr + used[i].size;
	      if (((used[i].addr >= addr) && (used[i].addr < end)) ||
		  ((used_end > addr) && (used_end <= end)) ||
		  ((used[i].addr <= addr) && (used_end >= end)))
		{
		  /* area overlaps used area */
#if DEBUG_ALLOC_INIT
		  INFO("heap 0x%08x-0x%08x\n",addr,end);
		  DMSG("  overlaps used area at 0x%08x-0x%08x\n",
		       used[i].addr,used_end);
#endif
		  
		  found = 0;
		  addr = used_end;
		  break;
		}
	    }
	}
    }
  else
    {
      /* test given map address */
      addr = l4rm_heap_start_addr;
      end = addr + size;

#if DEBUG_ALLOC_INIT
      INFO("\n");
      DMSG("  testing heap map area 0x%08x-0x%08x\n",addr,end);
#endif
	  
      found = 1;
      for (i = 0; i < num_used; i++)
	{
	  used_end = used[i].addr + used[i].size;
	  if (((used[i].addr >= addr) && (used[i].addr < end)) ||
	      ((used_end > addr) && (used_end <= end)) ||
	      ((used[i].addr <= addr) && (used_end >= end)))
	    {
	      /* area overlaps used area */
	      Msg("heap 0x%08x-0x%08xoverlaps used area at 0x%08x-0x%08x\n",
		  addr,end,used[i].addr,used_end);
	      
	      found = 0;
	      addr = used_end;
	      break;
	    }
	}
    }

  if (!found)
    {
      /* no heap area found */
      Panic("L4RM heap: no map area found!");
      return -1;
    }

  heap_start = addr;

  /* setup page list */
  for (i = 0; i < NUM_PAGES; i++)
    pages[i] = 0;
  heap_next_page = 0;

  /* set allocation mode */
  if (have_l4env)
    {
      use_l4env = 1;
      dsm_id = l4rm_get_dsm();
      
      /* open heap dataspace */
      ret = l4dm_mem_open(dsm_id,L4RM_HEAP_DS_INIT_PAGES * L4_PAGESIZE,
			  L4_PAGESIZE,L4DM_PINNED,"L4RM heap",&heap_ds);
      if (ret < 0)
	{
	  Panic("L4RM heap: open dataspace at %x.%x failed: %s (%d)!",
                dsm_id.id.task,dsm_id.id.lthread,l4env_errstr(ret),ret);
	  return -1;
	}

      /* mark first pages available */
      for (i = 0; i < L4RM_HEAP_DS_INIT_PAGES; i++)
	pages[i] = 1;
    }
  else
    {
      use_l4env = 0;
      sigma0_id = l4rm_get_sigma0(); 
    }

#if DEBUG_ALLOC_INIT
  INFO("\n  heap at 0x%08x, max %d pages\n",heap_start,NUM_PAGES);
  if (have_l4env)
    DMSG("  L4Env mode, heap dataspace %u at %x.%x\n",heap_ds.id,
	 heap_ds.manager.id.task,heap_ds.manager.id.lthread);
  else
    DMSG("  sigma0 mode, sigma0 at %x.%x\n",sigma0_id.id.task,
	 sigma0_id.id.lthread);
#endif

  return 0;
}

/*****************************************************************************/
/**
 * \brief  Register heap area
 *	
 * \return 0 on success, -1 if registration failed.
 *
 * Register the virtual memory area used for the heap. It cannot be done
 * in l4rm_init_alloc because at that point the region list and AVL tree 
 * are not yet initialized.
 */
/*****************************************************************************/ 
int
l4rm_heap_register(void)
{
  int ret,area;

  if (use_l4env)
    {
      /* attach heap dataspace */
      ret = l4rm_direct_attach_to_region(&heap_ds,(void *)heap_start,
					 L4RM_MAX_HEAP_SIZE,0,L4DM_RW);
      if (ret < 0)
	{
	  Panic("L4RM heap: attach heap dataspace failed: %s (%d)!",
		l4env_errstr(ret),ret);
	  return -1;
	}

#if DEBUG_ALLOC_INIT
      l4dm_ds_show(&heap_ds);
#endif
    }
  else
    {
      /* reserve heap area */
      ret = l4rm_direct_area_reserve_region(heap_start,L4RM_MAX_HEAP_SIZE,
					    0,&area);
      if (ret < 0)
	{
	  Panic("L4RM heap: reserve heap area failed: %s (%d)!",
		l4env_errstr(ret),ret);
	  return -1;
	}
    }

#if DEBUG_ALLOC_INIT
  l4rm_show_region_list();
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Allocate heap page
 * 
 * \return pointer to page, NULL if allocation failed.
 */
/*****************************************************************************/ 
void *
l4rm_heap_alloc(void)
{
  int page = heap_next_page;

  if (page >= NUM_PAGES)
    {
      ERROR("L4RM: heap overflow!");
      return NULL;
    }
  heap_next_page++;

  /* allocate page */
  if (use_l4env)
    return __l4env_allocate(page);
  else
    return __sigma0_allocate(page);
}

/*****************************************************************************/
/**
 * \brief  Add thread to heap clients
 * 
 * \param  thread        Thread id
 */
/*****************************************************************************/ 
void
l4rm_heap_add_client(l4_threadid_t client)
{
  int ret;

  /* share dataspace */
  ret = l4dm_share(&heap_ds,client,L4DM_RW | L4DM_RESIZE);
  if (ret < 0)
    Error("L4RM heap: add thread %x.%x to heap dataspace clients failed: "
	  "%s (%d)!",client.id.task,client.id.lthread,l4env_errstr(ret),ret);
}

/*****************************************************************************/
/**
 * \brief  Remove client from heap clients
 * 
 * \param  thread        Thread id
 */
/*****************************************************************************/ 
void
l4rm_heap_remove_client(l4_threadid_t client)
{
  l4dm_revoke(&heap_ds,client,L4DM_ALL_RIGHTS);
}
