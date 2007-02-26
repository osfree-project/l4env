/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/stacks.c
 * \brief  Stack handling.
 *
 * \date   08/31/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Stack Management.
 * We try to reserve a special address area for our stacks. If we get that
 * area, we can calculate the stack index from the stack pointer. If we
 * don't get that area (e.g. because we run in a small address space),
 * we map the stacks to any suitable address but cannot calculate the 
 * stack index from the stack pointer.
 * (A way to avoid that would be to align the stack, store the stack index
 * at the top of the stack and mask the stack pointer accordingly to access
 * the index. However, this does not work with stacks allocated by the user,
 * there is no way to distinguish between a properly aligned stack and a
 * user allocated stack.)
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard */
#include <stdio.h>

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__stacks.h"
#include "__memory.h"
#include "__tcb.h"
#include "__debug.h"

/*****************************************************************************
 *** global structures
 *****************************************************************************/

/**
 * Use stack area.
 *
 * Set to 1 if the reservation of the stack area succeeded. */
int l4th_have_stack_area = 0;

/* stack area */
l4_addr_t l4th_stack_area_start;         ///< start address of the stack area
l4_addr_t l4th_stack_area_end;           ///< end address of the stack area

static l4_uint32_t l4th_stack_area_id;   ///< area id of the stack area

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Setup stack area.
 * 
 * \return 0 on success
 *
 * Try to reserve the default stack area at region mapper.
 */
/*****************************************************************************/ 
int
l4th_stack_init(void)
{
  int ret;
  l4_addr_t area_addr;
  l4_size_t area_size;
  
  /* get stack area size */
  area_size = l4thread_max_threads * l4thread_max_stack;

  LOGdL(DEBUG_STACK_INIT, "stack setup:\n" \
        " %d threads, stack max. %zu bytes, stack area size 0x%08zx",
        l4thread_max_threads, l4thread_max_stack, area_size);

  if (l4thread_stack_area_addr == -1)
    {
      /* Try to reserve stack area, if this area is not available, stacks are
       * mapped to any suitable address. In that case l4thread_myself() can't 
       * calculate the tcb index from the stack pointer, it must search through 
       * the whole tcb table. */
      ret = l4rm_direct_area_reserve(area_size,
				     L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC,
				     &area_addr, &l4th_stack_area_id);
      if (ret < 0)
	{
	  /* failed to reserve stack area */
	  LOG_printf("l4thread: Warning, stack area not available!\n");
	  l4th_have_stack_area = 0;
	  l4th_stack_area_start = L4_MAX_ADDRESS;
	  l4th_stack_area_end = L4_MAX_ADDRESS;
	}
      else
	{
	  /* got stack area */
	  l4th_have_stack_area = 1;
	  l4th_stack_area_start = area_addr;
	  l4th_stack_area_end = area_addr + area_size - 1;
	  
#if DEBUG_STACK_INIT
	  LOG_printf(" using stack area <0x%08lx-0x%08lx>\n",
                 l4th_stack_area_start, l4th_stack_area_end);
#endif
	}
    }
  else
    {
#if DEBUG_STACK_INIT
      LOG_printf(" trying area at 0x%08lx\n", l4thread_stack_area_addr);
#endif

      /* reserve given stack map area */
      ret = l4rm_direct_area_reserve_region(l4thread_stack_area_addr, area_size,
					    L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC,
					    &l4th_stack_area_id);
      if (ret < 0)
	{
	  Panic("l4thread: specified stack area not available "\
		"(0x%08lx-0x%08lx)!", l4thread_stack_area_addr,
		l4thread_stack_area_addr + area_size);
	  return -1;
	}
      else
	{
	  /* got stack area */
	  l4th_have_stack_area = 1;
	  l4th_stack_area_start = l4thread_stack_area_addr;
	  l4th_stack_area_end = l4thread_stack_area_addr + area_size - 1;
	  
#if DEBUG_STACK_INIT
	  LOG_printf(" using stack area <0x%08lx-0x%08lx>\n",
                 l4th_stack_area_start, l4th_stack_area_end);
#endif
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Allocate stack.
 * 
 * \param  index         Stack index
 * \param  size          Stack size
 * \param  flags         Thread create flags
 * \param  owner         Stack owner
 * \param  desc          Target memory descriptor
 *
 * \return 0 on success, error code otherwise (see l4th_alloc_pages()).
 */
/*****************************************************************************/ 
int
l4th_stack_allocate(int index, l4_size_t size, l4_uint32_t flags,
		    l4_threadid_t owner, l4th_mem_desc_t * desc)
{
  l4_addr_t map_addr;
  int ret;

  /* align stack size to page size */
  size = l4_round_page(size);

  if (l4th_have_stack_area && (size <= l4thread_max_stack)) 
    {
      /* allocate stack in stack area */

      /* on x86 stack grows downwards, so we must map the stack at the end 
       * of the map area */
      map_addr = l4th_stack_area_start + 
	index * l4thread_max_stack + (l4thread_max_stack - size);

      LOGdL(DEBUG_STACK_ALLOC, 
            "allocating in stack area, index %d, addr 0x%08lx, size %zu",
            index, map_addr, size);

      ret = l4th_pages_allocate(size, map_addr, l4th_stack_area_id, owner,
				"L4thread stack", flags,desc);
    }
  else
    {
      /* allocating stack somewhere */
      LOGdL(DEBUG_STACK_ALLOC,
            "allocating stack somewhere, size %zu", size);
      
      ret = l4th_pages_allocate(size, VM_FIND_REGION, VM_DEFAULT_AREA,
                                owner, "L4thread stack", flags, desc);
    }

#if DEBUG_STACK_ALLOC
  if (ret < 0)
    LOGL("error allocating stack (%d)!", ret);
  else
    LOGL("stack at 0x%08lx", desc->map_addr);
#endif

  /* done */
  return ret;
}

/*****************************************************************************/
/**
 * \brief  Release stack.
 * 
 * \param  desc          Memory descriptor.
 *
 * Release the stack described by \a desc.
 */
/*****************************************************************************/ 
void
l4th_stack_free(l4th_mem_desc_t * desc)
{
  /* free stack */
  l4th_pages_free(desc);
}

/*****************************************************************************
 *** API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Return stack address
 * 
 * \param   thread       Thread id
 * \retval  stack_low    Stack address low 
 * \retval  stack_high   Stack address high
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid thread id
 */
/*****************************************************************************/ 
int
l4thread_get_stack(l4thread_t thread, l4_addr_t * low, l4_addr_t * high)
{
  l4th_tcb_t * tcb;

  /* get tcb */
  tcb = l4th_tcb_get(thread);
  if (tcb == NULL)
    /* invalid thread */
    return -L4_EINVAL;

  *low = tcb->stack.map_addr;
  *high = tcb->stack.map_addr + tcb->stack.size - 1;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief   Return stack address of current thread
 * 
 * \retval  stack_low    Stack address low 
 * \retval  stack_high   Stack address high
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  current thread not found in thread table
 */
/*****************************************************************************/ 
int
l4thread_get_stack_current(l4_addr_t * low, l4_addr_t * high)
{
  l4th_tcb_t * tcb;

  /* get TCB */
  tcb = l4th_tcb_get_current();
  if (tcb == NULL)
    /* current not found */
    return -L4_EINVAL;

  *low = tcb->stack.map_addr;
  *high = tcb->stack.map_addr + tcb->stack.size - 1;

  /* done */
  return 0;
}


