
#include "init_mem.h"
#include "memmap.h"
#include "globals.h"

#include <l4/sys/ktrace.h>
#include <l4/cxx/iostream.h>
#include <l4/sys/kip.h>
#include <l4/sys/kdebug.h>

using L4::Kip::Mem_desc;

void
init_memory(l4_kernel_info_t *info)
{
  if (info->version >> 24 != 0x87 /*KIP_VERSION_FIASCO*/ )
    {
      L4::cout << PROG_NAME": is designed to run on FIASCO only\n";
      enter_kdebug("FATAL");
    }

  iomem.add_free(Region(0, ~0UL));
  Mem_desc *md  = Mem_desc::first(info);
  Mem_desc *end = md + Mem_desc::count(info);

  for (; md < end; ++md)
    {
      if (md->is_virtual())
        continue;

      Mem_desc::Mem_type type = md->type();
      unsigned long end = l4_round_page(md->end())-1;
      unsigned long start = l4_trunc_page(md->start());
      switch (type)
	{
	case Mem_desc::Conventional:
	  Mem_man::ram()->add_free(Region(start, end));
	  iomem.alloc(Region(start, end, sigma0_taskno));
	  continue;
	case Mem_desc::Reserved:
	  Mem_man::ram()->alloc(Region(start, end, sigma0_taskno));
	  iomem.alloc(Region(start, end, sigma0_taskno));
	  break;
	case Mem_desc::Bootloader:
	  iomem.alloc(Region(start, end, sigma0_taskno));
	  Mem_man::ram()->alloc(Region(start, end, root_taskno));
	  break;
	case Mem_desc::Dedicated:
	case Mem_desc::Arch:
	  iomem.add_free(Region(start,end));
	  Mem_man::ram()->alloc(Region(start, end, sigma0_taskno));
	  break;
	default:
	  break;
	}
    }
}

