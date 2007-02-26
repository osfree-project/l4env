/* $Id$ */
/**
 * \file	sigma0/lib/src/iomem.c
 * \brief	map any page using sigma0 protocol
 *
 * \date	02/2006
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/ipc.h>
#include <l4/sigma0/sigma0.h>


/**
 * Map one page anonymous memory.
 *
 * \param pager          pager implementing the Sigma0 protocol
 * \param map_area       virtual address of the map area
 * \param log2_map_size  size of the map area
 * \return           #0  on success
 *                  -#2  IPC error
 *                  -#3  no fpage received
 *                  -#5  invalid size (old protocol)
 */
int
l4sigma0_map_anypage(l4_threadid_t pager, l4_addr_t map_area, 
		     unsigned log2_map_size, l4_addr_t *base)
{
  int error;
  l4_umword_t fpage;
  l4_msgdope_t result;

  error = l4_ipc_call(pager,
		      L4_IPC_SHORT_MSG, SIGMA0_REQ_FPAGE_ANY,
		        l4_fpage(0, L4_LOG2_PAGESIZE, 0, 0).fpage,
		      L4_IPC_MAPMSG(map_area, log2_map_size),
		      base, &fpage, L4_IPC_NEVER, &result);

  if (error)
    return -2;

  if (!fpage || !l4_ipc_fpage_received(result))
    return -3;

  return 0;
}
