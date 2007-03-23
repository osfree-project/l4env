/*!
 * \file	iomem.c
 * \brief	Handling of I/O memory mapped memory
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>
#ifdef ARCH_x86
#include <l4/generic_io/libio.h>
#include "pci.h"
#endif
#include <l4/sigma0/sigma0.h>

#include "iomem.h"

int con_hw_use_l4io;

static l4_threadid_t my_task_pager_id = L4_INVALID_ID;

static void
get_my_task_pager_id(void)
{
  /* get region manager's pager */
  my_task_pager_id = l4_thread_ex_regs_pager(l4rm_region_mapper_id());
}

int
map_io_mem(l4_addr_t paddr, l4_size_t size, int cacheable,
	   const char *id, l4_addr_t *vaddr)
{
  int error;
  l4_uint32_t rg;
  l4_offs_t offset;

  if (!con_hw_use_l4io)
    {
      offset = paddr - l4_trunc_superpage(paddr);
      size   = l4_round_superpage(size + offset);
      paddr  = l4_trunc_superpage(paddr);

      if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, vaddr, &rg)))
	Panic("Error %d reserving region size=%dMB for %s mem",
	      error, size>>20, id);

      LOG_printf("Mapping I/O %s mem "l4_addr_fmt" => "l4_addr_fmt
                 "+%06lx [%dkB]\n",
	         id, paddr+offset, *vaddr, offset, size>>10);

      if (l4_is_invalid_id(my_task_pager_id))
	get_my_task_pager_id();

      if (cacheable)
	LOG_printf("Warning: Cannot setup WC MTRR, use l4io server\n");

      if ((error = l4sigma0_map_iomem(my_task_pager_id, paddr, *vaddr, size,
				      cacheable ? 1 : 0)))
	{
	  switch (error)
	    {
	    case -1: Panic("This cannot happen");
	    case -2: Panic("IPC error mapping I/O %s mem", id);
	    case -3: Panic("No fpage received mapping I/O %s mem", id);
	    case -4: Panic("I/O %s memory address is below 2GB (0x80000000),\n"
			   "don't know how to map it as device super I/O "
			   "page", id);
	    }
	}

      *vaddr += offset;
    }
  else /* use l4io */
    {
#ifdef ARCH_x86
      if ((*vaddr = l4io_request_mem_region(paddr, size,
                                            cacheable
					      ? L4IO_MEM_WRITE_COMBINED : 0)) == 0)
	Panic("Can't request mem region from l4io.");

      LOG_printf("Mapped I/O %s mem  "l4_addr_fmt" => "l4_addr_fmt" [%dkB] via l4io\n",
                 id, paddr, *vaddr, size >> 10);
#else
      Panic("Use of l4io not supported.");
#endif
    }

  return 0;
}

void
unmap_io_mem(l4_addr_t addr, l4_size_t size, const char *id, l4_addr_t vaddr)
{
  if (!con_hw_use_l4io)
    {
      l4_addr_t vend   = vaddr + size;
      l4_addr_t vaddr1 = vaddr;

      vaddr &= L4_SUPERPAGEMASK;
      do
	{
	  l4_fpage_unmap(l4_fpage(vaddr, L4_LOG2_SUPERPAGESIZE, 0, 0),
			 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
	  vaddr += L4_SUPERPAGESIZE;
	} while (vaddr < vend);

      if (l4rm_area_release_addr((void*)l4_trunc_superpage(vaddr1)))
	Panic("Error releasing region "l4_addr_fmt"-"l4_addr_fmt, 
	    vaddr1, vaddr1+size);

      LOG_printf("Unmapped I/O %s mem\n", id);
    }
  else
    {
#ifdef ARCH_x86
      int error;

      if ((error = l4io_release_mem_region(addr, size)) < 0)
	Panic("Error %d releasing region "l4_addr_fmt"-"l4_addr_fmt" at l4io",
	    error, addr, addr+size);
      LOG_printf("Unmapped I/O %s mem via l4io\n", id);
#else
      Panic("Use of l4io not supported.");
#endif
    }
}
