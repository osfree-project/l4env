#include "ioports.h"
#include "mem_man.h"
#include "globals.h"

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#include <l4/cxx/iostream.h>

static Mem_man io_ports;

void init_io_ports(l4_kernel_info_t *info)
{
  io_ports.add_free(Region::kr(0, 64*1024));
}

void dump_io_ports()
{
  L4::cout << "IO PORTS--------------------------\n";
  io_ports.dump();
}

bool handle_io_page_fault(unsigned long d1, unsigned long d2, unsigned owner,
    void *&msg, unsigned long &rd1, unsigned long &rd2)
{
  if (l4_is_io_page_fault(d1))
    {
      unsigned port, size;
      port = ((l4_fpage_t&)d1).iofp.iopage;
      size = ((l4_fpage_t&)d1).iofp.iosize;

      unsigned long i = io_ports.alloc(Region::bs(port, 1 << size, owner));
      if (i == port)
	{
	  rd2 = l4_iofpage(port, size, 0).fpage;
	  rd1 = 0;
	  msg = L4_IPC_SHORT_FPAGE;
	}
      return true;
    }
  return false;
}

