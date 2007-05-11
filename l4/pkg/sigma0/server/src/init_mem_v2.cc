#include "init_mem.h"
#include "memmap.h"

#include <l4/sys/ktrace.h>
#include <l4/cxx/iostream.h>
#include <l4/sys/kip.h>

using L4::Kip::Mem_desc;

void
init_memory(l4_kernel_info_t *info)
{
  if (l4_info->version >> 24 == 0x87 /*KIP_VERSION_FIASCO*/ )
    {
      char kip_syscalls = l4_info->kip_sys_calls;
      tbuf_status = fiasco_tbuf_get_status_phys();

      L4::cout << "  Found Fiasco: KIP syscalls: " 
	<< (kip_syscalls ? "yes\n" : "no\n");
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
	  break;
	case Mem_desc::Reserved:
	case Mem_desc::Dedicated:
	  iomem.alloc(Region(start, end, sigma0_taskno));
	  Mem_man::ram()->alloc(Region(start, end, sigma0_taskno));
	  break;
	case Mem_desc::Bootloader:
	  iomem.alloc(Region(start, end, sigma0_taskno));
	  Mem_man::ram()->alloc(Region(start, end, root_taskno));
	  break;
	case Mem_desc::Arch:
	case Mem_desc::Shared:
	  iomem.add_free(Region(start,end));
	  Mem_man::ram()->alloc(Region(start, end, sigma0_taskno));
	  break;
	default:
	  break;
	}
    }
}
