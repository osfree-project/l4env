/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/pagefault.c
 * \brief  Region mapper pagefault handling. 
 *
 * \date   06/03/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/consts.h>
#include <l4/util/macros.h>
#include <l4/dm_generic/dm_generic.h>

#include "l4rm-server.h"

/* private includes */
#include "__region.h"
#include "__config.h"
#include "__debug.h"

#define DW_ADDR 0
#define DW_EIP  1

/** region mapper service thread id, set in libl4rm.c */
extern l4_threadid_t l4rm_service_id;

/*****************************************************************************
 *** L4RM internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Handle pagefault.
 * 
 * \param  src_id        source thread id
 * \param  buffer        Dice message buffer
 * \param  _ev           Dice environment
 *
 * Handle pagefault. Find the VM region the pagefault address belongs to
 * and ask its dataspace manager to pagein the appropriate page. On success,
 * the pagefault is handled and the IDL code has to reply a dummy short
 * message to the faulting thread only.
 */
/*****************************************************************************/ 
l4_int32_t
l4rm_handle_pagefault(CORBA_Object src_id, 
                      l4_rm_msg_buffer_t *buffer,
                      CORBA_Environment *_ev)
{
  l4rm_region_desc_t * region;
  l4_addr_t addr = 0;
  l4_addr_t eip = 0;
  int rw;
  l4_snd_fpage_t snd_fpage;
  CORBA_Environment env = dice_default_environment;
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

  addr = DICE_GET_DWORD(buffer, DW_ADDR);
  eip = DICE_GET_DWORD(buffer, DW_EIP);
  rw = addr & 2;

  if (!l4_task_equal(*src_id, l4rm_service_id))
    {
      printf("L4RM: blocked pagefault message from outside ("IdFmt")!\n",
             IdStr(*src_id));
      return DICE_NO_REPLY;
    }

#if DEBUG_PAGEFAULT
  if (rw)
    printf("[PF] write at 0x%08x, eip 0x%08x, src %x.%x\n",
           addr,eip,src_id->id.task,src_id->id.lthread);
  else
    printf("[PF] read at 0x%08x, eip 0x%08x, src %x.%x\n",
           addr,eip,src_id->id.task,src_id->id.lthread);
#endif

  if (l4_is_io_page_fault(addr))
    {
      l4_fpage_t io = (l4_fpage_t){fpage: addr};
      if (io.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE)
	Panic("[IO PF] all ports (cli/sti) eip 0x%08x, src %x.%x, "
	      "not implemented\n",
              eip,src_id->id.task,src_id->id.lthread);
      else
	Panic("[IO PF] 0x%04x-0x%04x eip 0x%08x, src %x.%x\n not implemented\n",
              io.iofp.iopage,io.iofp.iopage+(1<<io.iofp.iosize)-1,
              eip,src_id->id.task,src_id->id.lthread);
    }

  /* lookup region */
  region = l4rm_find_used_region(addr);
  if (region != NULL)
    {
      LOGdL(DEBUG_PAGEFAULT,"region %d at %x.%x",region->ds.id,
            region->ds.manager.id.task,region->ds.manager.id.lthread);
      
      /* call dataspace manager */
      offset = addr - region->start + region->offs;
      env.rcv_fpage = l4_fpage(addr & L4_PAGEMASK, L4_LOG2_PAGESIZE,0,0);
      ret = if_l4dm_generic_fault_call(&(region->ds.manager),
                                       region->ds.id,offset,&snd_fpage,&env);
      if (env.major != CORBA_NO_EXCEPTION)
	{
	  printf("error: %d (ipc: %x)\n", env.repos_id, env.ipc_error);
	  Panic("pagefault exception");
	}
      if (ret < 0)
	{	  
	  printf("Unresolved pagefault in thread %x.%x at addr %p, eip %p\n",
                 src_id->id.task, src_id->id.lthread, (void*)addr, (void*)eip);
	  printf("l4rm: dataspace at %08x+%#x\n", region->start,
                 region->end-region->start);
	  printf("l4rm: dataspace manager call failed (ret %d)\n",ret);
	  printf("dataspace %d, manager at %x.%02x",region->ds.id,
                 region->ds.manager.id.task,region->ds.manager.id.lthread);
	  Panic("pagefault");
	}
    }
  else
    {
      if ((addr & L4_PAGEMASK) == 0)
	printf("[PF] %s at 0x%08x, eip 0x%08x, src %x.%x\n\n",
               rw?"write":"read",addr,eip,src_id->id.task,src_id->id.lthread);
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
	  error = l4_ipc_call(l4rm_task_pager_id,
				   L4_IPC_SHORT_MSG, addr | rw, eip,
				   L4_IPC_MAPMSG(addr, L4_LOG2_PAGESIZE),
                                   &dummy, &dummy,
				   L4_IPC_NEVER, &result);
	  if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	    break;
	}
      if (error)
	Panic("L4RM: map ipc failed "
	      "(%x.%02x error=0x%02x pfa=0x%08x eip=0x%08x)",
              src_id->id.task, src_id->id.lthread, error, addr | rw, eip);

      if (!l4_ipc_fpage_received(result))
	Panic("L4RM: no fpage received "
	      "(region not found -- asked pager %x.%x)\n"
	      "      (tid=%x.%02x result=0x%08x pfa=0x%08x eip=0x%08x)",
	      l4rm_task_pager_id.id.task, l4rm_task_pager_id.id.lthread,
	      src_id->id.task, src_id->id.lthread,
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
	printf("[PF] write at 0x%08x, eip 0x%08x, src %x.%x\n",
               addr,eip,src_id->id.task,src_id->id.lthread);
      else
	printf("[PF] read at 0x%08x, eip 0x%08x, src %x.%x\n",
               addr,eip,src_id->id.task,src_id->id.lthread);
      Panic("L4RM: unknown pagefault!");
#endif /* L4RM_FORWARD_PAGEFAULTS */
    }

  /* this is not necessary, but this way we _know_ what the
   * response to the client is 
   */
  DICE_GET_DWORD(buffer, 0) = 0;
  DICE_GET_DWORD(buffer, 1) = 0;
#ifdef L4API_l4x0
  DICE_GET_DWORD(buffer, 2) = 0;
#endif
  DICE_SET_SHORTIPC_COUNT(buffer);
  
  /* done */
  return DICE_REPLY;
}
