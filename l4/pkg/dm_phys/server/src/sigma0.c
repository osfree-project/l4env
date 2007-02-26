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

#include "__debug.h"

/* L4 includes */
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>
#include <l4/sys/syscalls.h>
#include <l4/env/env.h>
#include <l4/util/macros.h>
#include <l4/sigma0/sigma0.h>
#include <l4/sigma0/kip.h>

/* private includes */
#include "__sigma0.h"
#include "__config.h"
#include "__pages.h"
#include "__kinfo.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * sigma0 id
 */
static l4_threadid_t sigma0_id = L4_INVALID_ID;

/**
 * Start of physical memory
 */
l4_addr_t ram_base;

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
dmphys_sigma0_init(void)
{
  l4_umword_t dummy;
  l4_threadid_t preempter;

  /* init some internal L4env stuff */
  preempter = sigma0_id = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
		    &preempter, &sigma0_id, &dummy, &dummy, &dummy);

  LOGdL(DEBUG_SIGMA0, "Sigma0 = "l4util_idfmt, l4util_idstr(sigma0_id));

  dmphys_kinfo_init_ram_base();

  ASSERT(ram_base == RAM_BASE);

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

  /* call sigma0 to map a page, the receive window is our whole map area */
  if ((error = l4sigma0_map_anypage(sigma0_id, DMPHYS_MEMMAP_START,
				    DMPHYS_MEMMAP_LOG2_SIZE, &base)))
    {
  enter_kdebug("stop");
      PANIC("DMphys: map page failed!");
      return NULL;
    }

  if (base < RAM_BASE)
    {
      PANIC("DMphys: received non-physical memory (%08lx)!", base);
      return NULL;
    }

  LOGdL(DEBUG_SIGMA0, "got a page base 0x%08lx, mapped at 0x%08lx",
        base, MAP_ADDR(base));

  /* return page map address */
  return (void *)MAP_ADDR(base);
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

  /* call sigma0 to map the page */
  page &= L4_PAGEMASK;

  if ((error = l4sigma0_map_mem(sigma0_id, page, MAP_ADDR(page), L4_PAGESIZE)))
    {
      switch (error)
	{
	case -2:
	  PANIC("DMphys: calling sigma0 failed!");
	  return -1;

	case -3:
	  /* sigma0 denied page */
	  LOGdL(DEBUG_SIGMA0, "requesting 4K-page at 0x%08lx: denied", page);
	  return -1;
	}
    }

  LOGdL(DEBUG_SIGMA0, "requesting 4K-page at 0x%08lx: success", page);

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
  l4_addr_t map_addr = MAP_ADDR(page);

  /* unmap */
  l4_fpage_unmap(l4_fpage(map_addr, L4_LOG2_PAGESIZE, 0, 0),
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

  /* call sigma0 to map the page */
  page &= L4_SUPERPAGEMASK;

  if ((error = l4sigma0_map_mem(sigma0_id, page, MAP_ADDR(page),
				L4_SUPERPAGESIZE)))
    {
      switch (error)
	{
	case -2:
	  PANIC("DMphys: calling sigma0 failed!");
	  return -1;

	case -3:
	  /* sigma0 denied page */
	  LOGdL(DEBUG_SIGMA0, "requesting 4M-page at 0x%08lx: denied", page);
	  return -1;

	case -6:
	  /* no 4M-page received, this can happen if the whole 4M-page is not
	   * available but the 4K-page at that address */
	  LOGdL(DEBUG_SIGMA0, "requesting 4M-page at 0x%08lx: failed", page);
	  return -1;
	}
    }

  LOGdL(DEBUG_SIGMA0, "requesting 4M-page at 0x%08lx: success", page);

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
  l4_addr_t map_addr = MAP_ADDR(page);

  /* unmap */
  l4_fpage_unmap(l4_fpage(map_addr, L4_LOG2_SUPERPAGESIZE, 0, 0),
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
  return l4sigma0_kip_map(sigma0_id);
}
