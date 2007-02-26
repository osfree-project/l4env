/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include <l4/crtx/crt0.h>
#include <l4/cxx/iostream.h>

#include "globals.h"
#include "mem_man.h"
#include "page_alloc.h"
#include "memmap.h"
#include "init.h"
#include "init_mem.h"
#include "ioports.h"
#include "mem_man_test.h"

/* started as the L4 sigma0 task from crt0.S */
void
init(l4_kernel_info_t *info)
{
  crt0_construction();

  l4_info = info;

  L4::cout << PROG_NAME": Hello!\n";
  L4::cout << "  KIP @ " << info << '\n';

  Page_alloc_base::init();

  init_memory(info);
  init_io_ports(info);

  //mem_man_test();

  L4::cout << "  allocated " << Page_alloc_base::total()/1024
           << "KB for maintenance structures\n";

  if (debug_memory_maps)
    {
      L4::cout << PROG_NAME": Dump of all resource maps\n"
               << "RAM:------------------------\n";
      Mem_man::ram()->dump();
      L4::cout << "IOMEM:----------------------\n";
      iomem.dump();
      dump_io_ports();
    }

  /* now start the memory manager */
  pager();
}
