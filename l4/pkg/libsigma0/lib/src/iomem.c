/* $Id$ */
/**
 * \file	sigma0/lib/src/iomem.c
 * \brief	map memory-mapped I/O memory using sigma0 protocol
 *
 * \date	02/2006
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/ipc.h>
#include <l4/sigma0/sigma0.h>


/**
 * Map memory-mapped I/O memory.
 *
 * \param pager  pager implementing the Sigma0 protocol
 * \param phys   physical address
 * \param virt   virtual address
 * \param size   size in bytes
 * \param cached != 0: map page cached, uncached otherwise
 * \return   #0  on success
 *          -#1  phys, virt, or size not aligned
 *          -#2  IPC error
 *          -#3  no fpage received
 *          -#4  bad physical address (old protocol only)
 *          -#7  cannot map I/O memory cached (old protocol only)
 */
int
l4sigma0_map_iomem(l4_threadid_t pager,
                   l4_addr_t phys, l4_addr_t virt, l4_addr_t size, int cached)
{
  l4_addr_t    d = L4_SUPERPAGESIZE;
  unsigned     l = L4_LOG2_SUPERPAGESIZE;
  l4_fpage_t   fpage;
  l4_umword_t  base;
  l4_msgdope_t result;
  l4_msgtag_t  tag;
  int error;

  if ((phys & (d-1)) || (size & (d-1)) || (virt & (d-1)))
    {
      l          = L4_LOG2_PAGESIZE;
      d          = L4_PAGESIZE;
    }

  if ((phys & (d-1)) || (size & (d-1)) || (virt & (d-1)))
    return -1;

  for (; size>0; phys+=d, size-=d, virt+=d)
    {
      do
	{
	  tag = l4_msgtag(L4_MSGTAG_SIGMA0, 0, 0, 0);
	  error = l4_ipc_call_tag(pager,
			          L4_IPC_SHORT_MSG,
			            cached ? SIGMA0_REQ_FPAGE_IOMEM_CACHED
				           : SIGMA0_REQ_FPAGE_IOMEM,
			            l4_fpage(phys, l, 0, 0).fpage, tag,
			          L4_IPC_MAPMSG(virt, l), &base, &fpage.fpage,
			          L4_IPC_NEVER, &result, &tag);
	}
      while (error == L4_IPC_SECANCELED || error == L4_IPC_SEABORTED);

      if (error)
	return -2;

      if (fpage.fpage == 0 || !l4_ipc_fpage_received(result))
	return -3;
    }

  return 0;
}
