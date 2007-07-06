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
#if defined(ARCH_x86) || defined(ARCH_amd64)
#include <l4/generic_io/libio.h>
#include "pci.h"
#endif

#include "iomem.h"

int
map_io_mem(l4_addr_t paddr, l4_size_t size, int cacheable,
	   const char *id, l4_addr_t *vaddr)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
   if ((*vaddr = l4io_request_mem_region(paddr, size,
                                         cacheable
                		        ? L4IO_MEM_WRITE_COMBINED : 0)) == 0)
       Panic("Can't request mem region from l4io.");

   LOG_printf("Mapped I/O %s mem  "l4_addr_fmt" => "l4_addr_fmt" [%zdkB] via l4io\n",
               id, paddr, *vaddr, size >> 10);
#else
   Panic("Use of l4io not supported.");
#endif

  return 0;
}

void
unmap_io_mem(l4_addr_t addr, l4_size_t size, const char *id, l4_addr_t vaddr)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  int error;

  if ((error = l4io_release_mem_region(addr, size)) < 0)
      Panic("Error %d releasing region "l4_addr_fmt"-"l4_addr_fmt" at l4io",
	    error, addr, addr+size);
  LOG_printf("Unmapped I/O %s mem via l4io\n", id);
#else
  Panic("Use of l4io not supported.");
#endif
}
