
#include <stdio.h>
#include <l4/env/errno.h>
#include <l4/util/rdtsc.h>
#include "init.h"
#include "ux.h"

l4_addr_t      hw_vid_mem_addr, hw_vid_mem_size;
l4_addr_t      hw_map_vid_mem_addr;
unsigned short hw_xres, hw_yres;
unsigned char  hw_bits;

static int init_done = 0;

int
con_hw_init(unsigned short xres, unsigned short yres, unsigned char *bits,
            unsigned short bpl,
	    l4_addr_t vid_mem_addr, l4_size_t vid_mem_size,
	    con_accel_t *accel, l4_uint8_t **map_vid_mem_addr)
{
  if (init_done)
    return -L4_EINVAL;

  l4_calibrate_tsc();

  hw_vid_mem_addr = vid_mem_addr;
  hw_vid_mem_size = vid_mem_size;
  hw_xres = xres;
  hw_yres = yres;
  hw_bits = *bits;

  if (!ux_probe(accel))
    {
      *map_vid_mem_addr = (void*)hw_map_vid_mem_addr;
      *bits = hw_bits;

      return 0;
    }
  return -L4_ENOTFOUND;
}
