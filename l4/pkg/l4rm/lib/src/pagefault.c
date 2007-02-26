/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/pagefault.c
 * \brief  Region mapper pagefault handling. 
 *
 * \date   06/03/2000
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
#include <l4/util/macros.h>
#include <l4/dm_generic/dm_generic.h>

/* private includes */
#include "__libl4rm.h"
#include "__region.h"
#include "__config.h"
#include "__debug.h"
 
/*****************************************************************************
 *** L4RM internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Handle pagefault.
 * 
 * \param  src_id        source thread id
 * \param  addr          pagefault address
 * \param  eip           instruction pointer
 *
 * Handle pagefault. Find the VM region the pagefault address belongs to
 * and call its dataspace manager.
 */
/*****************************************************************************/ 
void 
l4rm_handle_pagefault(l4_threadid_t src_id, 
		      l4_addr_t addr, 
		      l4_addr_t eip)
{
  l4rm_region_desc_t * region;
  int rw = addr & 2;
  l4_snd_fpage_t snd_fpage;
  sm_exc_t exc;
  int ret;
  l4_offs_t offset;
#if L4RM_FORWARD_PAGEFAULTS
#if L4RM_FORWARD_PAGEFAULTS_BY_IPC
  int error, dummy;
  l4_msgdope_t result;
  extern l4_threadid_t l4rm_task_pager_id;
#else
  char c;
#endif
#endif

#if DEBUG_PAGEFAULT
  if (rw)
    DMSG("[PF] write at 0x%08x, eip 0x%08x, src %x.%x\n",
	 addr,eip,src_id.id.task,src_id.id.lthread);
  else
    DMSG("[PF] read at 0x%08x, eip 0x%08x, src %x.%x\n",
	 addr,eip,src_id.id.task,src_id.id.lthread);
#endif

  if (l4_is_io_page_fault(addr))
    {
      l4_fpage_t io = (l4_fpage_t){fpage: addr};
      if (io.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE)
	Panic("[IO PF] all ports (cli/sti) eip 0x%08x, src %x.%x, "
	      "not implemented\n",
	    eip,src_id.id.task,src_id.id.lthread);
      else
	Panic("[IO PF] 0x%04x-0x%04x eip 0x%08x, src %x.%x\n not implemented\n",
	    io.iofp.iopage,io.iofp.iopage+(1<<io.iofp.iosize)-1,
	    eip,src_id.id.task,src_id.id.lthread);
    }

  /* lookup region */
  region = l4rm_find_used_region(addr);
  if (region != NULL)
    {
#if DEBUG_PAGEFAULT
      INFO("region %d at %x.%x\n",region->ds.id,
	   region->ds.manager.id.task,region->ds.manager.id.lthread);
#if 0
      enter_kdebug("-");
#endif
#endif

      /* call dataspace manager */
      offset = addr - region->start + region->offs;
      ret = if_l4dm_generic_fault(region->ds.manager,
				  l4_fpage(addr & L4_PAGEMASK,
					   L4_LOG2_PAGESIZE,0,0),
				  region->ds.id,offset,&snd_fpage,&exc);
      if (ret < 0)
	{	  
	  Msg("Unresolved pagefault in thread %x.%x at addr %p, eip %p\n",
	  	src_id.id.task, src_id.id.lthread, (void*)addr, (void*)eip);
	  Msg("l4rm: dataspace at %08x+%#x\n", region->start,
	  			            region->end-region->start);
	  Msg("l4rm: dataspace manager call failed (ret %d)\n",ret);
	  Msg("dataspace %d, manager at %x.%x (0x%08x:0x%08x)",region->ds.id,
	      region->ds.manager.id.task,region->ds.manager.id.lthread,
	      region->ds.manager.lh.high,region->ds.manager.lh.low);
	  Panic("pagefault");
	}
    }
  else
    {
      if ((addr & L4_PAGEMASK) == 0)
	Msg("[PF] %s at 0x%08x, eip 0x%08x, src %x.%x\n\n",
	    rw?"write":"read",addr,eip,src_id.id.task,src_id.id.lthread);
#if L4RM_FORWARD_PAGEFAULTS
#if L4RM_FORWARD_PAGEFAULTS_BY_IPC
      addr &= L4_PAGEMASK;
      if (rw)
	/* if writable page requested, the page can still be mapped ro */
	l4_fpage_unmap(l4_fpage(addr, L4_LOG2_PAGESIZE,
				L4_FPAGE_RW, L4_FPAGE_MAP),
		       L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
      for (;;)
	{
	  /* we may get l4_thread_ex_regs'ed ... */
	  error = l4_i386_ipc_call(l4rm_task_pager_id,
				   L4_IPC_SHORT_MSG, addr | rw, eip,
				   L4_IPC_MAPMSG(addr, L4_LOG2_PAGESIZE),
				     &dummy, &dummy,
				   L4_IPC_NEVER, &result);
	  if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	    break;
	}
      if (error)
	Panic("L4RM: map ipc failed (%x.%02x error=0x%02x pfa=0x%08x eip=0x%08x)",
	    src_id.id.task, src_id.id.lthread, error, addr | rw, eip);

      if (!l4_ipc_fpage_received(result))
	Panic("L4RM: no fpage received (region not found -- asked pager %x.%x)\n"
	      "      (tid=%x.%02x result=0x%08x pfa=0x%08x eip=0x%08x)",
	      l4rm_task_pager_id.id.task, l4rm_task_pager_id.id.lthread,
	      src_id.id.task, src_id.id.lthread,
	      result.msgdope, addr | rw, eip);
#else
      if (rw)
	{
	  asm volatile ("orl $0, (%0)" 
			: /* nothing out */
			: "r" (addr)
			);
	}
      else
	{
	  c = *(volatile char *)addr; 
	}
#endif /* L4RM_FORWARD_PAGEFAULTS_BY_IPC */
#else /* ! L4RM_FORWARD_PAGEFAULTS */
      if (rw)
	Msg("[PF] write at 0x%08x, eip 0x%08x, src %x.%x\n",
	    addr,eip,src_id.id.task,src_id.id.lthread);
      else
	Msg("[PF] read at 0x%08x, eip 0x%08x, src %x.%x\n",
	    addr,eip,src_id.id.task,src_id.id.lthread);
      Panic("L4RM: unknown pagefault!");
#endif /* L4RM_FORWARD_PAGEFAULTS */
    }

  /* done */
  return;
}
