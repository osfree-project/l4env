/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/map.c
 * \brief  Generic dataspace manager client library, map implementation
 *
 * \date   01/08/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/bitops.h>
#include <l4/util/l4_macros.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>
#include "__debug.h"
#include "__map.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Map dataspace region
 * 
 * \param  ds            Dataspace descriptor
 * \param  offs          Offset in dataspace
 * \param  size          Region size
 * \param  rcv_addr      Receive window address
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 * \param  flags	 Map flags
 * \retval fpage_addr    Map address of receive flexpage 
 * \retval fpage_size    Size of received flexpage
 *
 * \return 0 on success (mapped page), error code otherwise:
 *         - -#L4_EIPC    IPC error calling dataspace manager
 *
 * Preconditions:
 * - correct receive window specification
 */
/*****************************************************************************/ 
int
__do_map(const l4dm_dataspace_t * ds, l4_offs_t offs, l4_size_t size, 
	 l4_addr_t rcv_addr, int rcv_size2, l4_offs_t rcv_offs, 
	 l4_uint32_t flags, l4_addr_t * fpage_addr, l4_size_t * fpage_size)
{
  int ret;
  l4_snd_fpage_t page;
  CORBA_Environment _env = dice_default_environment;

  LOGdL(DEBUG_DO_MAP, "ds %u at "l4util_idfmt", offset 0x"l4_addr_fmt"\n" \
        " receive window at 0x"l4_addr_fmt", size2 %d, offset 0x"l4_addr_fmt"",
        ds->id, l4util_idstr(ds->manager), offs, rcv_addr, rcv_size2, rcv_offs);

  /* set receive fpage */
  _env.rcv_fpage = l4_fpage(rcv_addr, rcv_size2, 0, 0);
  /* do map call */
  ret = if_l4dm_generic_map_call(&(ds->manager), ds->id, offs, size, rcv_size2,
                                 rcv_offs, flags, &page,&_env);
  if ((ret < 0) || DICE_HAS_EXCEPTION(&_env))
    {
      if (ret < 0)
	return ret;
      else
	return -L4_EIPC;
    }

  /* analyze received fpage */
  LOGdL(DEBUG_DO_MAP, "got fpage at 0x"l4_addr_fmt", size2 %d\n" \
        " snd_base 0x"l4_addr_fmt", mapped to 0x"l4_addr_fmt,
        (l4_addr_t)(page.fpage.fp.page << L4_LOG2_PAGESIZE), page.fpage.fp.size,
        page.snd_base, rcv_addr + page.snd_base);

  *fpage_addr = rcv_addr + page.snd_base;
  *fpage_size = 1UL << page.fpage.fp.size;

  /* done */
  return 0;
}

/*****************************************************************************
 *** libdm_generic API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Map dataspace region (IDL wrapper)
 * 
 * \param  ds            Dataspace descriptor
 * \param  offs          Offset in dataspace
 * \param  size          Region size
 * \param  rcv_addr      Receive window address
 * \param  rcv_size2     Receive window size (log2)
 * \param  rcv_offs      Offset in receive window
 * \param  flags         Flags:
 *                       - #L4DM_RO          map read-only
 *                       - #L4DM_RW          map read/write
 *                       - #L4DM_MAP_PARTIAL allow partial mappings 
 *                       - #L4DM_MAP_MORE    if possible, map more than the 
 *                                           specified dataspace region
 * \retval fpage_addr    Map address of receive fpage
 * \retval fpage_size    Size of receive fpage
 *
 * \return 0 on success (got fpage), error code otherwise:
 *         - -#L4_EIPC         IPC error calling dataspace manager
 *         - -#L4_EINVAL       invalid dataspace id or map / receive window 
 *                             size
 *         - -#L4_EINVAL_OFFS  invalid dataspace / receive window offset
 *         - -#L4_EPERM        permission denied
 *
 * For a detailed description of #L4DM_MAP_PARTIAL and #L4DM_MAP_MORE
 * see l4dm_map_pages().
 */
/*****************************************************************************/ 
int
l4dm_map_pages(const l4dm_dataspace_t * ds, l4_offs_t offs, l4_size_t size, 
	       l4_addr_t rcv_addr, int rcv_size2, l4_offs_t rcv_offs, 
	       l4_uint32_t flags, l4_addr_t * fpage_addr, 
	       l4_size_t * fpage_size)
{
  int ret;

  /* do map */
  ret = __do_map(ds, offs, size, rcv_addr, rcv_size2, rcv_offs, flags,
		 fpage_addr, fpage_size);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      LOG_printf("ds %u at "l4util_idfmt", offset 0x"l4_addr_fmt"\n", ds->id,
             l4util_idstr(ds->manager), offs);
      LOG_printf("receive window at 0x"l4_addr_fmt
	     ", size2 %d, offset 0x"l4_addr_fmt"\n",
             rcv_addr, rcv_size2, rcv_offs);
      LOGL("libdm_generic: map page failed: %d!", ret);
#endif
    }

  return ret;
}
