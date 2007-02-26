#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <l4/util/port_io.h>

#define inb(port) l4util_in8(port)
#define outb(value, port) l4util_out8(value, port)

#define inb_p(port) ({ l4util_iodelay(); l4util_in8(port); })
#define outb_p(value, port) ({ l4util_iodelay(); l4util_out8(value, port); })

#endif
