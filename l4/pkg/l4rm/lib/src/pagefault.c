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
#include <l4/env/errno.h>
#include <l4/dm_generic/dm_generic.h>

#include "l4rm-server.h"

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__config.h"
#include "__debug.h"

#define DW_ADDR 0
#define DW_IP   1

/** region mapper service thread id, set in libl4rm.c */
extern l4_threadid_t l4rm_service_id;

/** the pager of the region mapper, set in libl4rm.c */
extern l4_threadid_t l4rm_task_pager_id;

/** this variable is set to != 0 if we received all I/O ports and therefore
 * have the right to set IOPL to 3 */
#if defined ARCH_x86 || ARCH_amd64
static int have_all_ioports;
#endif

/** pagefault callback function */
static l4rm_pf_callback_fn_t pf_callback;

/** unknown fault callback function */
static l4rm_unknown_fault_callback_fn_t unknown_fault_callback;

/** forward exception for unhandled pagefaults */
static int exception_on_unhandled_pf = 0;

/*****************************************************************************/
/**
 * \brief  Unkown pagefault
 * 
 * \param  addr          Pagefault address
 * \param  ip            Instruction pointer
 * \param  src_id        Source thread 
 *
 * \return Reply type
 */
/*****************************************************************************/ 
static int
__unknown_pf(l4_addr_t addr, l4_addr_t ip, CORBA_Object src_id)
{
  if (pf_callback != NULL)
    return pf_callback(addr, ip, *src_id);
  else if (exception_on_unhandled_pf)
    return L4RM_REPLY_EXCEPTION;
  else
    {
      LOG_printf("L4RM: [PF] %s at 0x"l4_addr_fmt", ip "l4_addr_fmt
	     ", src "l4util_idfmt"\n", (addr & 2) ? "write" : "read",
	     addr & ~3, ip, l4util_idstr(*src_id));
#if PANIC_ON_UNHANDLED_PF
      Panic("unhandled page fault");
#endif

      return L4RM_REPLY_NO_REPLY;
    }
}

static int
__handle_unknown_fault(l4_msgtag_t tag, l4_addr_t addr, l4_addr_t ip,
                       CORBA_Object src_id)
{
  if (unknown_fault_callback)
    return unknown_fault_callback(tag, l4_utcb_get(), *src_id);

  LOG_printf("L4RM: unknown exception src="l4util_idfmt
             " exc=%ld data=%lx,%lx\n",
             l4util_idstr(*src_id), l4_msgtag_label(tag), addr, ip);
  return L4RM_REPLY_NO_REPLY;
}

/*****************************************************************************/
/**
 * \brief  Handle pagefault to I/O port space
 * 
 * \param  addr          Pagefault address
 * \param  ip            Instruction pointer
 * \param  src_id        Pagefault source thread 
 */
/*****************************************************************************/ 
static inline int
__handle_iopf(l4_addr_t addr, l4_addr_t ip, CORBA_Object src_id)
{
#if defined ARCH_x86 || ARCH_amd64
  int error;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  l4_msgtag_t tag;

  addr &= ~3;

#if DEBUG_IO_PAGEFAULT
  {
    l4_fpage_t io = (l4_fpage_t){fpage: addr};
    if (io.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE)
      LOG("L4RM: [IOPF] all ports ip "l4_addr_fmt", src "
	  l4util_idfmt, ip, l4util_idstr(*src_id));
    else
      LOG("L4RM: [IOPF] %04x-%04x ip "l4_addr_fmt", src "l4util_idfmt,
	  io.iofp.iopage, io.iofp.iopage + (1 << io.iofp.iosize) - 1,
	  ip, l4util_idstr(*src_id));
  }
#endif

  for (;;)
    {
      /* we may get l4_thread_ex_regs'ed ... */
      tag = l4_msgtag(L4_MSGTAG_IO_PAGE_FAULT, 0, 0, 0);
      error = l4_ipc_call_tag(l4rm_task_pager_id,
                              L4_IPC_SHORT_MSG, addr, ip, tag,
                              L4_IPC_IOMAPMSG(0, L4_WHOLE_IOADDRESS_SPACE),
                              &dw0, &dw1,
                              L4_IPC_NEVER, &result, &tag);
      if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	break;
    }

  if (EXPECT_FALSE(error))
    {
      LOG_printf("L4RM: map ipc failed " \
             "(I/O fault -- asked pager "l4util_idfmt", error=0x%02x)\n",
             l4util_idstr(l4rm_task_pager_id), error);
      return __unknown_pf(addr, ip, src_id);
    }

  /* Don't check if we got a flexpage if we still (sometimes) had all
   * I/O ports */
  if (EXPECT_FALSE(!have_all_ioports && !dw1))
    return L4RM_REPLY_EMPTY;

  /* If we requested all I/O ports and we got them then we run at IOPL 3
   * from now on. Any other thread of our task still running at IOPL 0 may
   * raise an I/O pagefault when it tries to execute cli or sti. Since we
   * own now all I/O ports, we only need to reply to such pagefaults and
   * the kernel raises the IOPL of that thread to 3. */
  if (addr == l4_iofpage(0, L4_WHOLE_IOADDRESS_SPACE, 0).fpage)
    have_all_ioports = 1;

  /* done */
  return result.md.fpage_received ? L4RM_REPLY_SUCCESS :  L4RM_REPLY_EMPTY;
#else
  return L4RM_REPLY_EMPTY;
#endif
}

/*****************************************************************************/
/**
 * \brief  Call dataspace manager to handle pagefault
 * 
 * \param  addr          Pagefault address
 * \param  ip            Instruction pointer
 * \param  region        Region descriptor
 * \param  src_id        Pagefault source thread
 *
 * \return Reply type
 */
/*****************************************************************************/
static inline int
__call_dm(l4_addr_t addr, l4_addr_t ip, l4rm_region_desc_t * region,
	  CORBA_Object src_id)
{
  DICE_DECLARE_ENV(env);
  l4_snd_fpage_t snd_fpage;
  l4_offs_t offset;
  int ret;

  LOGdL(DEBUG_PAGEFAULT, "dataspace %d at "l4util_idfmt,
        region->data.ds.ds.id, l4util_idstr(region->data.ds.ds.manager));

  /* call dataspace manager */
  offset = addr - region->start + region->data.ds.offs;
  env.rcv_fpage = l4_fpage(addr & L4_PAGEMASK, L4_LOG2_PAGESIZE, 0, 0);
  ret = if_l4dm_generic_fault_call(&region->data.ds.ds.manager,
                                   region->data.ds.ds.id, offset,
                                   &snd_fpage, &env);
  if (EXPECT_FALSE(DICE_HAS_EXCEPTION(&env) || (ret < 0)))
    {
      LOG_printf("L4RM: dataspace at 0x"l4_addr_fmt"-0x"l4_addr_fmt
	     ", id %d at "l4util_idfmt"\n",
             region->start, region->end, region->data.ds.ds.id,
             l4util_idstr(region->data.ds.ds.manager));
      if (ret < 0)
        LOG_printf("L4RM: dataspace manager call failed (\"%s\")\n",
                   l4env_errstr(ret));
      else
        LOG_printf("L4RM: map call error %d (ipc: %x)\n",
               DICE_EXCEPTION_MINOR(&env), DICE_IPC_ERROR(&env));

      return __unknown_pf(addr, ip, src_id);
    }

  /* done */
  return L4RM_REPLY_EMPTY;
}

/*****************************************************************************/
/**
 * \brief  Forward pagefault to another pager
 * 
 * \param  addr          Pagefault address
 * \param  ip            Instruction pointer
 * \param  region        Region descriptor
 * \param  src_id        Source thread
 *
 * \return Reply type
 */
/*****************************************************************************/ 
static inline int
__forward_pf(l4_addr_t addr, l4_addr_t ip, l4rm_region_desc_t * region,
             CORBA_Object src_id)
{
  int error;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  l4_msgtag_t tag;

  LOGdL(DEBUG_PAGEFAULT, "forward to pager "l4util_idfmt, 
        l4util_idstr(region->data.pager.pager));

  for (;;)
    {
      /* we may get l4_thread_ex_regs'ed ... */
      tag = l4_msgtag(L4_MSGTAG_PAGE_FAULT, 0, 0, 0);
      error = l4_ipc_call_tag(region->data.pager.pager,
                              L4_IPC_SHORT_MSG, addr, ip, tag,
                              L4_IPC_MAPMSG(addr, L4_LOG2_PAGESIZE),
                              &dw0, &dw1,
                              L4_IPC_NEVER, &result, &tag);
      if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	break;
    }

  if (EXPECT_FALSE(error))
    {
      LOG_printf("L4RM: map ipc failed " \
             "(page fault -- asked pager "l4util_idfmt", error=0x%02x)\n",
             l4util_idstr(region->data.pager.pager), error);
      return __unknown_pf(addr, ip, src_id);
    }

  if (EXPECT_FALSE(!dw1))
    {
      LOG_printf("L4RM: no fpage received "
             "(page fault -- asked pager "l4util_idfmt", result 0x"l4_addr_fmt
	     ")\n", l4util_idstr(region->data.pager.pager), result.msgdope);
      return __unknown_pf(addr, ip, src_id);
    }

  /* done */
  return L4RM_REPLY_EMPTY;
}

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
l4rm_handle_pagefault(CORBA_Object src_id, l4_msgtag_t *tag,
                      l4_rm_msg_buffer_t *buffer,
                      CORBA_Server_Environment *_ev)
{
  l4rm_region_desc_t * region;
  l4_addr_t addr, ip;
  int rw, reply;

  addr = DICE_GET_DWORD(buffer, DW_ADDR);
  ip   = DICE_GET_DWORD(buffer, DW_IP);
  rw   = addr & 2;

  if (EXPECT_FALSE(!l4_task_equal(*src_id, l4rm_service_id)))
    {
      LOG_printf("L4RM: blocked PF/Exceptio message from outside ("
                 l4util_idfmt")!", l4util_idstr(*src_id));
      return DICE_NO_REPLY;
    }

#if DEBUG_PAGEFAULT
  LOG_printf("L4RM: [PF] %s at "l4_addr_fmt", ip "l4_addr_fmt", src "
         l4util_idfmt"\n", rw ? "write" : "read", addr & ~3, ip,
	 l4util_idstr(*src_id));
#endif

  if (EXPECT_TRUE(l4_msgtag_is_page_fault(*tag)))
    {
      /* lookup region */
      region = l4rm_find_used_region(addr);
      if (EXPECT_TRUE(region != NULL))
        {
          switch(REGION_TYPE(region))
            {
            case REGION_DATASPACE:
              /* call dataspace manager */
              reply = __call_dm(addr, ip, region, src_id);
              break;

            case REGION_PAGER:
              /* call external pager */
              reply = __forward_pf(addr, ip, region, src_id);
              break;

            case REGION_EXCEPTION:
              /* forward pagefault exception */
              reply = L4RM_REPLY_EXCEPTION;
              break;

            case REGION_BLOCKED:
              /* pagefault to blocked region, whats that? */
              LOG_printf("L4RM: page fault in blocked region 0x"l4_addr_fmt
		     "-0x"l4_addr_fmt"\n", region->start, region->end);
              reply = __unknown_pf(addr, ip, src_id);
              break;

            default:
              /* Ooops, unknwown region type */
              LOG_printf("L4RM: page fault in unknown region 0x"l4_addr_fmt
		     "-0x"l4_addr_fmt", flags 0x%08x\n",
                     region->start, region->end, region->flags);
              reply = __unknown_pf(addr, ip, src_id);
            }
        }
      else
        /* no entry in region list */
        reply = __unknown_pf(addr, ip, src_id);
    }
  else if (EXPECT_FALSE(l4_msgtag_is_io_page_fault(*tag)))
    reply = __handle_iopf(addr, ip, src_id);
  else
    reply = __handle_unknown_fault(*tag, addr, ip, src_id);

  DICE_SET_SHORTIPC_COUNT(buffer);

  if (EXPECT_TRUE(reply == L4RM_REPLY_EMPTY))
    {
      /* set reply message */
      DICE_GET_DWORD(buffer, 0) = 0;
      DICE_GET_DWORD(buffer, 1) = 0;
      return DICE_REPLY;
    }
  else if (reply == L4RM_REPLY_SUCCESS)
    {
      /* set reply message */
      DICE_GET_DWORD(buffer, 0) = 1;
      DICE_GET_DWORD(buffer, 1) = 1;
      return DICE_REPLY;
    }
  else if (reply == L4RM_REPLY_EXCEPTION)
    {
      /* set reply message */
      DICE_GET_DWORD(buffer, 0) = -1;
      DICE_GET_DWORD(buffer, 1) = -1;
      return DICE_REPLY;
    }
  else
    return DICE_NO_REPLY;
}

/*****************************************************************************/
/**
 * \brief  Set callback function for unkown pagefaults
 *
 * \param  callback      Callback function
 */
/*****************************************************************************/ 
void
l4rm_set_unkown_pagefault_callback(l4rm_pf_callback_fn_t callback)
{
  pf_callback = callback;
}

/*****************************************************************************/
/**
 * \brief  Set callback function for unkown pagefaults
 *
 * \param  callback      Callback function
 */
/*****************************************************************************/
void
l4rm_set_unkown_fault_callback(l4rm_unknown_fault_callback_fn_t callback)
{
  unknown_fault_callback = callback;
}

/*****************************************************************************/
/**
 * \brief  Enable exception forward for unkown pagefaults
 */
/*****************************************************************************/ 
void
l4rm_enable_pagefault_exceptions(void)
{
  exception_on_unhandled_pf = 1;
}

/*****************************************************************************/
/**
 * \brief  Disable exception forward for unkown pagefaults
 */
/*****************************************************************************/ 
void
l4rm_disable_pagefault_exceptions(void)
{
  exception_on_unhandled_pf = 0;
}
