
#include <l4/env/errno.h>
#include "init.h"

int con_hw_use_l4io;

void
con_hw_set_l4io(int l4io)
{
}

int
con_hw_init(unsigned short xres, unsigned short yres, unsigned char *bits, 
	    unsigned int vid_mem_addr, unsigned int vid_mem_size,
	    con_accel_t *accel, l4_uint8_t **map_vid_mem_addr)
{
  /* Fiasco-UX does not have any hardware acceleration */
  return -L4_ENOTFOUND;
}
