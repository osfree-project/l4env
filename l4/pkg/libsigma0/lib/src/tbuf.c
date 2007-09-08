/* $Id$ */
/**
 * \file	sigma0/lib/src/tbuf.c
 * \brief	map tracebuffer descriptor using sigma0 protocol
 *
 * \date	02/2006
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sigma0/sigma0.h>

/**
 * Map the Fiasco tracebuffer status descriptor using the Sigma0 protocol.
 *
 * \param pager  pager implementing the Sigma0 protocol
 * \param virt   virtual address the descriptor should be mapped to
 * \return  #0   on success
 *         -#2   IPC error
 *         -#3   no fpage received
 */
int
l4sigma0_map_tbuf(l4_threadid_t pager, l4_addr_t virt)
{
  l4_umword_t base;
  l4_fpage_t fpage;
  l4_msgdope_t result;
  int error;
  l4_msgtag_t tag = l4_msgtag(L4_MSGTAG_SIGMA0, 0, 0, 0);

  error = l4_ipc_call_tag(pager, L4_IPC_SHORT_MSG, SIGMA0_REQ_TBUF, 0, tag,
                          L4_IPC_MAPMSG(virt, L4_LOG2_PAGESIZE),
		          &base, &fpage.fpage, L4_IPC_NEVER, &result, &tag);

  if (error)
    return -2;

  if (fpage.fpage == 0 || !l4_ipc_fpage_received(result))
    return -3;

  return 0;
}
