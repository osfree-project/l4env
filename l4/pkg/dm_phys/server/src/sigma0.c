/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/sigma0.c
 * \brief  Communication with the sigma0 server.
 *
 * \date   08/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>
#include <l4/sys/syscalls.h>
#include <l4/env/env.h>
#include <l4/util/macros.h>

/* private includes */
#include "__sigma0.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * sigma0 id
 */
static l4_threadid_t sigma0_id = L4_INVALID_ID;

/*****************************************************************************
 *** DMphys internal functions 
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Setup Sigma0 communication.
 *	
 * \return 0 on success, -1 if setup failed (Sigma0 not found)
 */
/*****************************************************************************/ 
int
dmhys_sigma0_init(void)
{
  l4_umword_t dummy;
  l4_threadid_t preempter;

  /* init some internal L4env stuff */
  preempter = sigma0_id = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
		    &preempter, &sigma0_id, &dummy, &dummy, &dummy);

  LOGdL(DEBUG_SIGMA0,"Sigma0 = %x.%x\n",
        sigma0_id.id.task,sigma0_id.id.lthread);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Request any page
 *	
 * \return pointer to mapped page, NULL if no page received.
 */
/*****************************************************************************/ 
void *
dmphys_sigma0_map_any_page(void)
{
  int error;
  l4_addr_t base;
  l4_fpage_t fpage;
  l4_msgdope_t result;

  /* call sigma0 to map a page, the receive window is our whole map area */
  error = l4_ipc_call(sigma0_id,L4_IPC_SHORT_MSG,0xFFFFFFFC,0,
			   L4_IPC_MAPMSG(DMPHYS_MEMMAP_START, 
					 DMPHYS_MEMMAP_LOG2_SIZE),
			   &base,&fpage.fpage,L4_IPC_NEVER,&result);
  if ((error) || (!l4_ipc_fpage_received(result)))
    {
      PANIC("DMphys: map page failed (result 0x%08x)!",result.msgdope);
      return NULL;
    }

  LOGdL(DEBUG_SIGMA0,"got page 0x%05x\n  base 0x%08x, mapped at 0x%08x",
        fpage.fp.page,base,DMPHYS_MEMMAP_START + base);
  
  /* return page map address */
  return (void *)(DMPHYS_MEMMAP_START + base);
}

/*****************************************************************************/
/**
 * \brief  Request a specific page.
 * 
 * \param  page          Phys. page address
 *	
 * \return 0 on success, -1 if mapping failed.
 */
/*****************************************************************************/ 
int
dmphys_sigma0_map_page(l4_addr_t page)
{
  int error;
  l4_addr_t base;
  l4_fpage_t fpage;
  l4_msgdope_t result;

  LOGdL(DEBUG_SIGMA0,"");

  /* call sigma0 to map the page */
  page &= L4_PAGEMASK;
  error = l4_ipc_call(sigma0_id,L4_IPC_SHORT_MSG,page,0,
			   L4_IPC_MAPMSG(DMPHYS_MEMMAP_START + page, 
					 L4_LOG2_PAGESIZE),
			   &base,&fpage.fpage,L4_IPC_NEVER,&result);
  if (error)
    {
      PANIC("DMphys: calling sigma0 failed (result 0x%08x)!",result.msgdope);
      return -1;
    }

  if ((fpage.fpage == 0) || (!l4_ipc_fpage_received(result)))
    {
      /* sigma0 denied page */
#if DEBUG_SIGMA0
      printf("  requesting 4K-page at 0x%08x: denied\n",page);
#endif
      return -1;
    }

#if DEBUG_SIGMA0
  printf("  requesting 4K-page at 0x%08x: success\n",page);
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Unmap page
 * 
 * \param  page          Phys. page address
 */
/*****************************************************************************/ 
void
dmphys_sigma0_unmap_page(l4_addr_t page)
{
  l4_addr_t map_addr = DMPHYS_MEMMAP_START + page;

  /* unmap */
  l4_fpage_unmap(l4_fpage(map_addr,L4_LOG2_PAGESIZE,0,0),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
}

/*****************************************************************************/
/**
 * \brief  Request a specific 4M-page
 * 
 * \param  page          Page address
 *	
 * \return 0 on success, -1 if mapping failed.
 */
/*****************************************************************************/ 
int
dmphys_sigma0_map_4Mpage(l4_addr_t page)
{
  int error;
  l4_addr_t base;
  l4_fpage_t fpage;
  l4_msgdope_t result;

  LOGdL(DEBUG_SIGMA0,"");

  /* call sigma0 to map the page */
  page &= L4_SUPERPAGEMASK;
  error = l4_ipc_call(sigma0_id,L4_IPC_SHORT_MSG,
			   page | 1, L4_LOG2_SUPERPAGESIZE << 2,
			   L4_IPC_MAPMSG(DMPHYS_MEMMAP_START + page,
					 L4_LOG2_SUPERPAGESIZE),
			   &base,&fpage.fpage,L4_IPC_NEVER,&result);

  if (error)
    {
      PANIC("DMphys: calling sigma0 failed (result 0x%08x)!",result.msgdope);
      return -1;
    }

  if ((fpage.fpage == 0) || (!l4_ipc_fpage_received(result)))
    {
      /* sigma0 denied page */
#if DEBUG_SIGMA0
      printf("  requesting 4M-page at 0x%08x: denied\n",page);
#endif
      return -1;
    }

  if (fpage.fp.size != L4_LOG2_SUPERPAGESIZE)
    {
      /* no 4M-page received, this can happen if the whole 4M-page is not
       * available but the 4K-page at that address */
#if DEBUG_SIGMA0
      printf("  requesting 4M-page at 0x%08x: failed (got %d)\n",
             page,fpage.fp.size);
#endif

      /* unmap page, the 4K-page must be mapped explicitly */
      fpage.fp.page += (DMPHYS_MEMMAP_START >> L4_LOG2_PAGESIZE);
      l4_fpage_unmap(fpage,L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

      return -1;
    }

#if DEBUG_SIGMA0
  printf("  requesting 4M-page at 0x%08x: success\n",page);
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Unmap 4M-page
 * 
 * \param  page          Phys. page address
 */
/*****************************************************************************/ 
void
dmphys_sigma0_unmap_4Mpage(l4_addr_t page)
{
  l4_addr_t map_addr = DMPHYS_MEMMAP_START + page;
  
  /* unmap */
  l4_fpage_unmap(l4_fpage(map_addr,L4_LOG2_SUPERPAGESIZE,0,0),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
}

/*****************************************************************************/
/**
 * \brief Map L4 kernel info page 
 * 
 * \return Pointer to kernel info page, NULL if mapped failed.
 */
/*****************************************************************************/ 
l4_kernel_info_t *
dmphys_sigma0_kinfo(void)
{
  int error;
  l4_addr_t base;
  l4_fpage_t fpage;
  l4_msgdope_t result;

  /* call sigma0 to map kernel info page */
  error = l4_ipc_call(sigma0_id,L4_IPC_SHORT_MSG,1,1,
			   L4_IPC_MAPMSG(DMPHYS_KINFO_MAP, L4_LOG2_PAGESIZE),
			   &base,&fpage.fpage,L4_IPC_NEVER,&result);
  if ((error) || (!l4_ipc_fpage_received(result)))
    {
      PANIC("DMphys: map kinfo page failed (result 0x%08x)!",result.msgdope);
      return NULL;
    }
  
  /* return pointer to kinfo page */
  return (l4_kernel_info_t *)DMPHYS_KINFO_MAP;
}
