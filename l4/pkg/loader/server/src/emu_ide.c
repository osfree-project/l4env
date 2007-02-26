/* Experimental stuff for emulating DMA of Linux ATA devices */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "pager.h"

#include <stdio.h>

#include <l4/crtx/ctor.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/port_io.h>

static void
ide_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	   l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  app_t *app = desc->app;

  if (ea.emu.write)
    {
      l4_umword_t offs = ea.emu.offs;
      switch (offs)
	{
	case 0x04:
	  if (desc->private_mem1_phys)
	    {
	      l4_uint32_t *table_src = 
		(l4_uint32_t*)addr_app_to_here(app, value);
	      l4_uint32_t *table_dst = 
		(l4_uint32_t*)desc->private_mem1;
	      int i;

	      for (i=0; i<L4_PAGESIZE/8; i++)
		{
		  l4_size_t size = table_src[1] & 0x7fffffff;

		  if (size == 0)
		    size = 64 << 10;
		  if (!addr_app_to_here(app, table_src[0]) ||
		      !addr_app_to_here(app, table_src[0]+size-1))
		    {
		      app_msg(app, "IDE buffer address %08x-%08x invalid", 
			  table_src[0], table_src[0]+size-1);
		      enter_kdebug("FATAL");
		    }
		  table_dst[0] = table_src[0];
		  table_dst[1] = table_src[1];
		  if (table_dst[1] & 0x80000000)
		    break;
		  table_dst += 2;
		  table_src += 2;
		}
	      if (i >= L4_PAGESIZE/8)
		{
		  enter_kdebug("DMA table overflow");
		}

	      l4util_out32((l4_uint32_t)desc->private_mem1_phys,
			   desc->phys_addr + ea.emu.offs);
	    }
	  else
	    {
	      // can't emulate because we have no page
	      l4util_out32(value, desc->phys_addr + ea.emu.offs);
	    }
	  break;
	default:
	  app_msg(app, "IDE write %08x value %08x", offs, value);
	  l4util_out32(value, desc->phys_addr + ea.emu.offs);
	  break;
	}
    }
  else
    app_msg(app, "IDE read?");
}

// special command
static void
ide_spec(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	 l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  if (ea.emu.size == 3)
    {
      // return device settings to client
      *dw1 = desc->phys_addr;
      *dw2 = desc->irq;
      *reply = L4_IPC_SHORT_MSG;
    }
}

static void
ide_init(emu_desc_t *desc)
{
  l4dm_mem_addr_t addrs[1];
  l4_size_t size;

  size = L4_PAGESIZE;
  desc->private_mem1 =
    (l4_addr_t)l4dm_mem_allocate_named(size, L4DM_CONTIGUOUS | 
					     L4DM_PINNED |
					     L4RM_MAP,
					     "ide DMA table");
  if (l4dm_mem_phys_addr((void*)desc->private_mem1, size, addrs, 1, &size) < 0)
    {
      printf("Error determining phys addr of ide_dma table\n");
      return;
    }
  if (size < L4_PAGESIZE)
    {
      printf("Psize %08x < L4_PAGESIZE\n", size);
      return;
    }
  printf("DMA table for %s at virt=%08x phys=%08x\n", 
      desc->name, (l4_addr_t)desc->private_mem1, addrs[0].addr);
  desc->private_mem1_phys = addrs[0].addr;
}

emu_desc_t emu_ide_1 =
{
//  .phys_addr = 0xb800,
  .phys_addr = 0xffa0,
  .phys_size = 8,
  .irq       = 0xff,
  .init      = ide_init,
  .handle    = ide_handle,
  .spec      = ide_spec,
  .name      = "ide channel 0"
};

emu_desc_t emu_ide_2 =
{
  .phys_addr = 0xb808,
  .phys_size = 8,
  .irq       = 0xff,
  .init      = ide_init,
  .handle    = ide_handle,
  .spec      = ide_spec,
  .name      = "ide channel 1"
};

static void
ide_register(void)
{
  emulate_register(&emu_ide_1, 1);
  emulate_register(&emu_ide_2, 2);
}

L4C_CTOR(ide_register, L4CTOR_AFTER_BACKEND);
