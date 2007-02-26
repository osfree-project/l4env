#ifndef SIGMA0_IO_PORTS_H__
#define SIGMA0_IO_PORTS_H__

#include <l4/sys/kernel.h>

void init_io_ports(l4_kernel_info_t *info);
bool handle_io_page_fault(unsigned long d1, unsigned long d2, unsigned owner,
                          void *&msg, unsigned long &rd1, unsigned long &rd2);

void dump_io_ports();

#endif /* ! SIGMA0_IO_PORTS_H__ */
