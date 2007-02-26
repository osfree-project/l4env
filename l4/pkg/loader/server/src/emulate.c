/* Some experimental code for emulating mmio of loader clients */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "pager.h"

#include <stdio.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/generic_io/libio.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/macros.h>
#include <l4/dm_mem/dm_mem.h>

static emu_desc_t *emu_desc[4];

// handle write pagefault to mmio register
int
handle_mmio_emu(app_t *app, emu_addr_t ea, l4_umword_t value,
		l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  l4_umword_t offs = ea.emu.offs;
  l4_umword_t size = 1 << ea.emu.size;
  emu_desc_t *desc;

  *reply = L4_IPC_SHORT_MSG;
  if (ea.emu.desc >= sizeof(emu_desc)/sizeof(emu_desc[0]))
    {
      app_msg(app, "invalid mmio emu desc #%d", ea.emu.desc);
      enter_kdebug("stop");
      return -L4_EINVAL;
    }

  desc = emu_desc[ea.emu.desc];

  if (desc->app != app)
    {
      if (desc->app == 0)
	// initialize app pointer
	desc->app = app;
      else
	{
	  app_msg(app, "%s not assigned to this application", desc->name);
	  enter_kdebug("stop");
	  return -L4_EINVAL;
	}
    }

  if (offs >= 0x00300000 && size > 1 && desc->spec)
    {
      /* special request */
      desc->spec(desc, ea, value, dw1, dw2, reply);
      return 0;
    }

  if (offs > desc->phys_size-size)
    {
      app_msg(desc->app, "invalid mmio offset %08x (size=%08x)", 
	      offs, desc->phys_size);
      enter_kdebug("stop");
      return -L4_EINVAL;
    }

  if (!desc->handle)
    {
      app_msg(desc->app, "no handler registered for desc #%d", ea.emu.desc);
      enter_kdebug("stop");
      return -L4_EINVAL;
    }

  desc->handle(desc, ea, value, dw1, dw2, reply);

  return 0;
}

// Check if application is allowed to map I/O adapter page writeable. Mapping
// is not allowed if the adapter page is under emulation.
void fastcall
check_mmio_emu(app_t *app, l4_umword_t pfa, l4_umword_t *dw2, void *reply)
{
  int i;
  l4_fpage_t fp = { .fpage = *dw2 };

  if (reply == L4_IPC_SHORT_FPAGE)
    {
      for (i=0; i<sizeof(emu_desc)/sizeof(emu_desc[0]); i++)
	{
	  if (   (pfa >= emu_desc[i]->phys_addr) 
	      && (pfa <  emu_desc[i]->phys_addr + emu_desc[i]->phys_size))
	    {
	      app_msg(app, "Write to %08x-%08x denied",
		  emu_desc[i]->phys_addr, 
		  emu_desc[i]->phys_addr+emu_desc[i]->phys_size);
	      fp.fp.write = 0;
	      break;
	    }
	}
    }

  *dw2 = fp.fpage;
}

/** Map mmio page into our address space with a size of 4MB. We map only
 * chunks of 4k to clients.
 * XXX Handle freeing of superpages when the client is killed. */
static int
map_mmio_page(emu_desc_t *desc)
{
  extern int use_l4io;
  int error;
  l4_msgdope_t result;
  l4_addr_t vm_addr, phys_addr;
  l4_offs_t offs;
  l4_uint32_t rg;

  if (!use_l4io)
    {
      phys_addr = l4_trunc_superpage(desc->phys_addr);
      offs      = desc->phys_addr - phys_addr;

      if ((error = l4rm_area_reserve(L4_SUPERPAGESIZE, L4RM_LOG2_ALIGNED,
				     &vm_addr, &rg)))
	{
	  /* should never fail as we can man around 768 superpages (3GB) */
	  printf("Error %d reserving I/O map area\n", error);
	  enter_kdebug("app_pager");
	  return -L4_ENOMEM;
	}

      if (phys_addr < 0x80000000)
	Panic("Cannot handle physical address %08x with Sigma0 protocol", 
	    phys_addr);

      for (;;)
	{
	  /* we could get l4_thread_ex_regs'd ... */
	  l4_umword_t dummy;
	  error = l4_ipc_call(rmgr_pager_id,
			      L4_IPC_SHORT_MSG, (phys_addr-0x40000000) | 2, 0,
			      L4_IPC_MAPMSG(vm_addr, L4_LOG2_SUPERPAGESIZE),
			        &dummy, &dummy,
			      L4_IPC_NEVER, &result);

	  if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	    break;
	}

      if (error || !l4_ipc_fpage_received(result))
	{
	  /* could only fail if already mapped to an other client */
	  printf("RMGR denied mapping of mmio page %08x at %08x\n",
	      phys_addr, vm_addr);
	  enter_kdebug("app_pager");
	  return -L4_ENOMEM;
	}
    }
  else
    {
      l4io_info_t *io_info_addr = (l4io_info_t*)-1;

      if (l4io_init(&io_info_addr, L4IO_DRV_INVALID))
	Panic("Can't initialize L4 I/O server");

      if (!(vm_addr = l4io_request_mem_region(desc->phys_addr, 
					      desc->phys_size, &offs)))
	Panic("Can't request mem region %08x size %08x from l4io.",
	    desc->phys_addr, desc->phys_size);

      printf("Mapped I/O mem %08x => %08x+%06x [%dkB] via l4io\n",
	  desc->phys_addr, vm_addr, offs, desc->phys_size >> 10);
    }

  desc->map_addr = vm_addr + offs;

  return 0;
}

int
emulate_init(void)
{
  int i;

  for (i=0; i<sizeof(emu_desc)/sizeof(emu_desc[0]); i++)
    {
      /* map in all i/o pages */
      if (emu_desc[i] && emu_desc[i]->phys_addr > 0x40000000)
	map_mmio_page(emu_desc[i]);

      if (emu_desc[i] && emu_desc[i]->init)
	emu_desc[i]->init(emu_desc[i]);
    }

  return 0;
}

void
emulate_register(emu_desc_t *desc, int desc_nr)
{
  if (desc_nr > sizeof(emu_desc)/sizeof(emu_desc[0]))
    {
      printf("emulation descriptor %d not available\n", desc_nr);
      return;
    }
  if (emu_desc[desc_nr] != 0)
    {
      printf("emulation descriptor %d already occupied\n", desc_nr);
      return;
    }
  
  emu_desc[desc_nr] = desc;
}
