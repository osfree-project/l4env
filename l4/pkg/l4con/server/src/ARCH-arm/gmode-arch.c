/* $Id$ */
/**
 * \file	con/server/src/ARCH-x86/gmode-arch.c
 * \brief	graphics mode initialization, x86 specific
 *
 * \date	2005
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <stdio.h>

#include <l4/sys/l4int.h>
#include <l4/arm_drivers/lcd.h>
#include <l4/util/macros.h>

#include "gmode.h"
#include "vc.h"

void
init_gmode(void)
{
  struct arm_lcd_ops *lcd;

  if (!(lcd = arm_lcd_probe()))
    {
      printf("Could not find LCD.\n");
      return;
    }

  printf("Using LCD driver: %s\n", lcd->get_info());

  gr_vmem_size     = lcd->get_video_mem_size();
  VESA_XRES        = lcd->get_screen_width();
  VESA_YRES        = lcd->get_screen_height();
  VESA_YRES_CLIENT = lcd->get_screen_height() - status_area;
  VESA_BITS        = lcd->get_bpp();
  VESA_BPL         = lcd->get_bytes_per_line();
  VESA_RES         = 0;
  if (VESA_BITS == 32)
    {
      VESA_RED_OFFS   = 16;
      VESA_RED_SIZE   = 8;
      VESA_GREEN_OFFS = 8;
      VESA_GREEN_SIZE = 8;
      VESA_BLUE_OFFS  = 0;
      VESA_BLUE_SIZE  = 8;
    }
  else
    {
      VESA_RED_OFFS   = 0;
      VESA_RED_SIZE   = 5;
      VESA_GREEN_OFFS = 5;
      VESA_GREEN_SIZE = 5;
      VESA_BLUE_OFFS  = 10;
      VESA_BLUE_SIZE  = 5;
    }

  gr_vmem         = lcd->get_fb();
  gr_vmem_maxmap  = gr_vmem + gr_vmem_size;
  vis_vmem        = gr_vmem;
  if (!gr_vmem)
    Panic("Could not setup video memory");

  lcd->enable();
}
