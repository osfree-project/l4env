#include "ioports.h"
#include "mem_man.h"

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

static Mem_man io_ports;

void init_io_ports(l4_kernel_info_t *info)
{
}

bool handle_io_page_fault(unsigned long d1, unsigned long d2, unsigned owner,
    void *&msg, unsigned long &rd1, unsigned long &rd2)
{
  return false;
}

void dump_io_ports()
{}

