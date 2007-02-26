/* Some experimental code for emulating mmio of loader clients */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "emu_speedo.h"
#include "emu_ide.h"
#include "emu_tulip.h"
#include "pager.h"

#include <stdio.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dm_mem/dm_mem.h>

// XXX must be same as in L4Linux
#undef  EMU_SPEEDO
#undef  EMU_IDE
#define EMU_TULIP

static emu_desc_t emu_desc[] =
{
#ifdef EMU_SPEEDO
    { .phys_addr=0xec000000, .size=L4_PAGESIZE,       
      .init=speedo_init, .handle=speedo_handle, .spec=speedo_spec,
      .name="eepro100", },
#endif
#ifdef EMU_IDE
    { .phys_addr=0x0000d800, .size=8, 
      .init=ide_init, .handle=ide_handle,
      .name="ide channel 0" },
#endif
#ifdef EMU_IDE
    { .phys_addr=0x0000d808, .size=8,
      .init=ide_init, .handle=ide_handle,
      .name="ide channel 1" },
#endif
#ifdef EMU_TULIP
    { .phys_addr=0x0000b800, .size=0x100,
      .init=tulip_init, .handle=tulip_handle, .spec=tulip_spec,
      .name="tulip" },
#endif
};

// handle write pagefault to mmio register
int
handle_mmio_emu(app_t *app, emu_addr_t ea, l4_umword_t value,
		l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  l4_umword_t offs = ea.emu.offs;
  l4_umword_t size = 1 << ea.emu.size;
  emu_desc_t *desc = emu_desc + ea.emu.desc;

  if (desc->app != app)
    {
      if (desc->app == 0)
	// initialize app pointer
	desc->app = app;
      else
	{
	  app_msg(app, "%s not assigned to this application", desc->name);
	  enter_kdebug("stop");
	  *reply = L4_IPC_SHORT_MSG;
	  return -L4_EINVAL;
	}
    }

  if ((offs >= 0x00300000) && (size > 1) && desc->spec)
    {
      /* special request: map shared area */
      desc->spec(desc, ea, value, dw1, dw2, reply);
      return 0;
    }

  if (desc >= emu_desc + (sizeof(emu_desc)/sizeof(emu_desc[0])))
    {
      app_msg(desc->app, "invalid mmio emu desc #%d", ea.emu.desc);
      enter_kdebug("stop");
      *reply = L4_IPC_SHORT_MSG;
      return -L4_EINVAL;
    }

  if (offs > desc->size-size)
    {
      app_msg(desc->app, "invalid mmio offset %08x (size=%08x)", 
	      offs, desc->size);
      enter_kdebug("stop");
      *reply = L4_IPC_SHORT_MSG;
      return -L4_EINVAL;
    }

  if (!desc->handle)
    {
      app_msg(desc->app, "no handler registered for desc #%d", ea.emu.desc);
      enter_kdebug("stop");
      *reply = L4_IPC_SHORT_MSG;
      return -L4_EINVAL;
    }

  desc->handle(desc, ea, value, dw1, dw2, reply);

  return 0;
}

// Check if application is allowed to map I/O adapter page writeable. Mapping
// is not allowed if the adapter page is under emulation.
void
check_mmio_emu(app_t *app, l4_umword_t pfa, l4_fpage_struct_t *fp, void *reply)
{
  int i;

  if (reply == L4_IPC_SHORT_FPAGE)
    {
      for (i=0; i<sizeof(emu_desc)/sizeof(emu_desc_t); i++)
	{
	  if (   (pfa >= emu_desc[i].phys_addr) 
	      && (pfa <  emu_desc[i].phys_addr + emu_desc[i].size))
	    {
	      app_msg(app, "Write to %08x-%08x forbidden because mediated",
		  emu_desc[i].phys_addr, 
		  emu_desc[i].phys_addr+emu_desc[i].size);
	      fp->write = 0;
	      break;
	    }
	}
    }
}

/** Map mmio page into our address space with a size of 4MB. We map only
 * chunks of 4k to clients.
 * XXX Handle freeing of superpages when the client is killed. */
static int
map_mmio_page(emu_desc_t *desc)
{
  int error;
  l4_msgdope_t result;
  l4_addr_t vm_addr;
  l4_addr_t phys_addr = l4_trunc_superpage(desc->phys_addr);
  l4_offs_t offs = desc->phys_addr - phys_addr;
  l4_uint32_t rm_area;

  if ((error = l4rm_area_reserve(L4_SUPERPAGESIZE, L4RM_LOG2_ALIGNED,
			         &vm_addr, &rm_area)))
    {
      /* should never fail as we can man around 768 superpages (3GB) */
      printf("Error %d reserving I/O map area\n", error);
      enter_kdebug("app_pager");
      return -L4_ENOMEM;
    }

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

  desc->map_addr = vm_addr + offs;

  return 0;
}

int
init_mmio_emu(void)
{
  int i;

  for (i=0; i<sizeof(emu_desc)/sizeof(emu_desc_t); i++)
    {
      /* map in all i/o pages */
      if (emu_desc[i].phys_addr > 0x40000000)
	map_mmio_page(emu_desc + i);

      if (emu_desc[i].init)
	emu_desc[i].init(emu_desc + i);
    }

  return 0;
}

