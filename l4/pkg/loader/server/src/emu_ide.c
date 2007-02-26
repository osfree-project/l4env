/* Experimental stuff for emulating DMA of Linux ATA devices */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "emu_ide.h"
#include "pager.h"

#include <stdio.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dm_mem/dm_mem.h>

void
ide_init(emu_desc_t *desc)
{
  l4dm_mem_addr_t addrs[1];
  l4_size_t psize;
  static int nr;

  desc->private_mem = l4dm_mem_allocate_named(L4_PAGESIZE,
					      L4DM_CONTIGUOUS | 
					      L4DM_PINNED |
					      L4RM_MAP,
					      "ide DMA table");
  if (l4dm_mem_phys_addr(desc->private_mem, L4_PAGESIZE, addrs, 1, &psize) < 0)
    {
      printf("Error determining phys addr of ide_dma table\n");
      return;
    }
  if (psize < L4_PAGESIZE)
    {
      printf("Psize %08x < L4_PAGESIZE\n", psize);
      return;
    }

  printf("DMA table for IDE channel %d at %08x\n", nr, addrs[0].addr);
  desc->private_mem_phys = (void*)addrs[0].addr;

  nr++;
}

void
ide_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	   l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  unsigned *table_src, *table_dst;
  int i;

  if (desc->private_mem_phys)
    {
      table_src = (unsigned*)addr_app_to_here(desc->app, value);
      table_dst = (unsigned*)desc->private_mem;
      
      for (i=0; i<L4_PAGESIZE/8; i++)
	{
	  table_dst[0] = table_src[0];
	  table_dst[1] = table_src[1];
	  table_src[0] = table_src[1] = 0;
	  if (table_dst[1] & 0x80000000)
	    break;
	  table_dst += 2;
	  table_src += 2;
	}
     
#ifdef DEBUG_IDE_DMA
      app_msg(desc->app, "handle_ide_dma(%08x => %04x, %d entries)",
	  value, desc->phys_addr + ea.emu.offs, i+1);
#endif

      asm volatile ("outl %%eax, (%%dx)\n\t"
		    : : "a"(desc->private_mem_phys), 
			"d"(desc->phys_addr + ea.emu.offs));
    }
  else
    {
      // can't emulate because we have no page
      asm volatile ("outl %%eax, (%%dx)\n\t" 
		    : : "a"(value), "d"(desc->phys_addr + ea.emu.offs));
    }
}

