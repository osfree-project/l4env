/*!
 * \file	init.c
 * \brief	init hardware accel stuff
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <stdio.h>
#include <l4/env/errno.h>

#include "init.h"
#include "pci.h"
#include "ati.h"
#include "ati128.h"
#include "matrox.h"
#include "riva.h"
#include "vmware.h"
#include "savage.h"

unsigned int   hw_vid_mem_addr, hw_vid_mem_size;
unsigned int   hw_map_vid_mem_addr;
unsigned short hw_xres, hw_yres;
unsigned char  hw_bits;
int            use_l4io;

static int init_done = 0;

int
con_hw_init(unsigned short xres, unsigned short yres, unsigned char *bits, 
	    unsigned int vid_mem_addr, unsigned int vid_mem_size, int l4io,
	    con_accel_t *accel, void **map_vid_mem_addr)
{
  if (init_done)
    return -L4_EINVAL;

  /* L4IO is pre-initialized */
  if (!l4io)
    pcibios_init();

  hw_vid_mem_addr = vid_mem_addr;
  hw_vid_mem_size = vid_mem_size;
  hw_xres = xres;
  hw_yres = yres;
  hw_bits = *bits;
  use_l4io = l4io;

  ati_register();
  ati128_register();
  matrox_register();
  riva_register();
  vmware_register();
  savage_register();

  if (pci_probe(accel)==0)
    {
      /* set by probe functions */
      printf("Backend scaler: %s, color keying: %s\n",
    	  (accel->caps & (ACCEL_FAST_CSCS_YV12|ACCEL_FAST_CSCS_YUY2)) 
	    ? (accel->caps & ACCEL_FAST_CSCS_YV12) ? "YV12" : "YUY2"
	    : "no",
	  (accel->caps & ACCEL_COLOR_KEY) ? "yes" : "no");
      *map_vid_mem_addr = (void*)hw_map_vid_mem_addr;
      *bits = hw_bits;
//      accel->caps &= ~(ACCEL_FAST_CSCS_YV12|ACCEL_FAST_CSCS_YUY2);
      return 0;
    }

  return -L4_ENOTFOUND;
}

