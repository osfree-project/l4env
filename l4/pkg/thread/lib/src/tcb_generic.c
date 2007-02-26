/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/tcb.c
 * \brief  Thread control block handling.
 *
 * \date   09/02/2000
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
#include <l4/sys/syscalls.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__tcb.h"
#include "__l4.h"
#include "__memory.h"
#include "__asm.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global structures
 *****************************************************************************/

/// thread control block table 
l4th_tcb_t * l4th_tcbs = NULL;

/// thread control block table memory descriptor
static l4th_mem_desc_t tcb_table;

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Setup tcb table.
 *
 * \return 0 on success, error code if setup failed
 */
/*****************************************************************************/ 
int
l4th_tcb_init(void)
{
  l4_size_t table_size,offs;
  int ret,i;

  /* allocate tcb table */
  table_size = l4_round_page(l4thread_max_threads * sizeof(l4th_tcb_t));

  if (l4thread_tcb_table_addr == -1)
    ret = l4th_pages_allocate(l4_round_page(table_size),VM_FIND_REGION,
			      VM_DEFAULT_AREA,L4_INVALID_ID,
			      "L4thread TCB table",
			      L4THREAD_CREATE_SETUP | L4THREAD_CREATE_PINNED,
			      &tcb_table);
  else
    ret = l4th_pages_allocate(l4_round_page(table_size),
			      l4thread_tcb_table_addr,
			      VM_DEFAULT_AREA,L4_INVALID_ID,
			      "L4thread TCB table",
			      L4THREAD_CREATE_SETUP | L4THREAD_CREATE_PINNED,
			      &tcb_table);


  if (ret < 0)
    {
      Error("l4thread: TCB table allocation failed: %s (%d)!",
	    l4env_errstr(ret),ret);
      return ret;
    }

#if DEBUG_TCB_INIT
  INFO("\n");
  DMSG("  tcb size = %d, tcb table size = %u\n",sizeof(l4th_tcb_t),table_size);
  DMSG("  ds %u at "IdFmt", mapped to 0x%08x\n",tcb_table.ds.id,
       IdStr(tcb_table.ds.manager),tcb_table.map_addr);
#endif

  /* map tcb table */
  offs = 0;
  while (offs < table_size)
    {
      ret = l4th_pages_map(&tcb_table,offs);
      if (ret < 0)
	{
	  Error("l4thread: map TCB table failed: %s (%d)!",
		l4env_errstr(ret),ret);
	  l4th_pages_free(&tcb_table);
	  return ret;
	}

      offs += L4_PAGESIZE;
    }
  l4th_tcbs = (l4th_tcb_t *)tcb_table.map_addr;

  /* setup tcb table */
  for (i = 0; i < l4thread_max_threads; i++)
    {
      l4th_tcbs[i].lock = L4LOCK_UNLOCKED;
      l4th_tcbs[i].state = TCB_UNUSED;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Mark TCB of thread reserved, the thread id will not be used to
 *         create new threads
 * 
 * \param  thread        Thread id
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid thread id
 *         - -#L4_EUSED   thread already used
 */
/*****************************************************************************/ 
int
l4th_tcb_reserve(l4thread_t thread)
{
  if ((thread < 0) || (thread >= l4thread_max_threads))
    return -L4_EINVAL;
  
  /* try to reserve tcb */
  if (cmpxchg16(&l4th_tcbs[thread].state,TCB_UNUSED,TCB_RESERVED))
    return 0;
  else
    return -L4_EUSED;
}

/*****************************************************************************/
/**
 * \brief  Return id of current thread (slow version)
 * 
 * \return id of current thread, #L4THREAD_INVALID_ID if not found.
 *
 * There are two ways to get the id of the current thread. The fast way
 * is to calculate the stack index (which is equal to the index in the tcb
 * table) from the stack pointer of the current thread. But this only works
 * if the stack was created by the thread library in a special area of the
 * address space (see stack.c). The slow way is to search the stack pointer
 * in the tcb table manually. 
 * 
 * The reason to avoid the L4 system call l4_myself() is that it is more 
 * expensive (at least compared to calculating the stack index), and might
 * become even more expensive for future versions of L4.
 */
/*****************************************************************************/ 
l4thread_t
l4th_tcb_get_current_id_slow(void)
{
  l4_addr_t esp = l4th_get_esp();
  l4thread_t thread;
  int i;

  /* first attempt: search stack pointer */
  for (i = 0; i < l4thread_max_threads; i++)
    {
      if (l4th_tcbs[i].state != TCB_UNUSED)
	{
	  if ((esp >= l4th_tcbs[i].stack.map_addr) &&
	      (esp < (l4th_tcbs[i].stack.map_addr + l4th_tcbs[i].stack.size)))
	    /* found */
	    return i;
	}
    }

  /* last attempt: ask L4 kernel
   * l4_myself might become really expensive in the future, so this is
   * the last attempt if nothing else works */
  thread = l4th_l4_from_l4id(l4th_l4_myself());
  if ((thread < 0) || (thread >= l4thread_max_threads) || 
      (l4th_tcbs[thread].state == TCB_UNUSED) ||
      (l4th_tcbs[thread].state == TCB_RESERVED))
    {
      Error("l4thread: current thread not found in TCB table!");
      return L4THREAD_INVALID_ID;
    }

  /* done */
  return thread;
}
