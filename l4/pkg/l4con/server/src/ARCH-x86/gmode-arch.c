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

#include <l4/sys/types.h>
#include <l4/env/mb_info.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/x86emu/int10.h>

#include "gmode.h"
#include "main.h"
#include "vc.h"
#include "con_hw/init.h"
#include "con_hw/iomem.h"

/** Try to detect and initialize the graphics card. */
static int
detect_hw(l4util_mb_vbe_ctrl_t *vbe, l4util_mb_vbe_mode_t *vbi)
{
  l4_addr_t vid_mem_addr;

  if (noaccel)
    return -L4_ENOTFOUND;

  vid_mem_addr = vbi->phys_base;
  gr_vmem_size = vbe->total_memory << 16;

  if (con_hw_init(VESA_XRES, VESA_YRES, &VESA_BITS, VESA_BPL,
		  vid_mem_addr, gr_vmem_size, &hw_accel, &gr_vmem)<0)
    {
      /* fall back to VESA support */
      printf("No supported accelerated graphics card detected\n");
      return -L4_ENOTFOUND;
    }

  fg_do_copy = hw_accel.copy;
  fg_do_fill = hw_accel.fill;
  fg_do_sync = hw_accel.sync;
  fg_do_drty = hw_accel.drty;

  if (hw_accel.caps & ACCEL_FAST_COPY)
    accel_caps |= L4CON_FAST_COPY;
  if (hw_accel.caps & ACCEL_FAST_CSCS_YV12)
    accel_caps |= L4CON_STREAM_CSCS_YV12;
  if (hw_accel.caps & ACCEL_FAST_CSCS_YUY2)
    accel_caps |= L4CON_STREAM_CSCS_YUY2;
  if (hw_accel.caps & ACCEL_POST_DIRTY)
    accel_caps |= L4CON_POST_DIRTY;
  if (hw_accel.caps & ACCEL_FAST_FILL)
    accel_caps |= L4CON_FAST_FILL;

  gr_vmem_maxmap = gr_vmem + VESA_YRES_CLIENT*VESA_BPL;
  vis_vmem       = gr_vmem;
  vis_offs       = 0;
  if (gr_vmem_maxmap >= gr_vmem+gr_vmem_size)
    {
      LOG_printf("Framebuffer too small??\n");
      gr_vmem_maxmap = gr_vmem+gr_vmem_size;
    }

  return 0;
}

/** Try to pan the memory */
static void
pan_vmem(void)
{
  l4_addr_t next_super_offs, start;
  int x, y, bpp;

  if (!pan)
    return;

  /* consider overflow on memory modes with more than 4MB */
  next_super_offs = l4_round_superpage(VESA_YRES_CLIENT * VESA_BPL);

  if (next_super_offs + status_area*VESA_BPL > gr_vmem_size)
    {
      printf("WARNING: Can't pan display: Only have %zdkB video memory "
	     "need %ldkB\n", gr_vmem_size >> 10, 
	     (next_super_offs + status_area*VESA_BPL) >> 10);
      return;
    }

  bpp = (VESA_BITS + 1) / 8;
  x   = (next_super_offs % VESA_BPL) / bpp;
  y   = (next_super_offs / VESA_BPL) - VESA_YRES_CLIENT;

  /* pan graphics card */
  if (hw_accel.pan)
    {
      /* using hw-specific function */
      hw_accel.pan(&x, &y);
    }
  else
    {
      /* using VESA BIOS call */
      if (x86emu_int10_pan((unsigned*)&x, (unsigned*)&y) < 0)
	return;
    }

  pan_offs_x     = x;
  pan_offs_y     = y;
  start          = y*VESA_BPL + x*bpp;

  gr_vmem_maxmap = gr_vmem + next_super_offs;
  vis_vmem       = gr_vmem + start;
  vis_offs       = start;

  printf("Display panned to %d:%d\n", y, x);
  panned = 1;
}

/** Initialize console driver */
void
init_gmode(void)
{
  l4util_mb_vbe_mode_t *vbi;
  l4util_mb_vbe_ctrl_t *vbe;

  /* 1) Mode switch */
  if (vbemode == 0 || x86emu_int10_set_vbemode(vbemode, &vbe, &vbi) != 0)
    {
      l4util_mb_info_t *mbi = l4env_multiboot_info;
      if (!(mbi->flags & L4UTIL_MB_VIDEO_INFO) || !(mbi->vbe_mode_info))
	Panic(           "Did not find VBE info block in multiboot info.\n"
	      "GRUB has to set the video mode with the vbeset command.\n"
	      "\n"
	      "Alternatively, try passing the --vbemode=<mode> switch.\n");

      vbe = (l4util_mb_vbe_ctrl_t*)(l4_addr_t)mbi->vbe_ctrl_info;
      vbi = (l4util_mb_vbe_mode_t*)(l4_addr_t)mbi->vbe_mode_info;
    }

  /* 2) Read graphics mode parameters from VESA controller/mode info */
  gr_vmem_size     = vbe->total_memory << 16;
  VESA_YRES        = vbi->y_resolution;
  VESA_YRES_CLIENT = VESA_YRES - status_area;
  VESA_XRES        = vbi->x_resolution;
  VESA_BITS        = vbi->bits_per_pixel;
  VESA_BPL         = vbi->bytes_per_scanline;
  VESA_RES         = 0;
  VESA_RED_OFFS    = vbi->red_field_position;
  VESA_RED_SIZE    = vbi->red_mask_size;
  VESA_GREEN_OFFS  = vbi->green_field_position;
  VESA_GREEN_SIZE  = vbi->green_mask_size;
  VESA_BLUE_OFFS   = vbi->blue_field_position;
  VESA_BLUE_SIZE   = vbi->blue_mask_size;

  printf("VESA reports %dx%d@%d %dbpl (%04x) [%zdkB]\n"
         "Color mapping: red=%d:%d green=%d:%d blue=%d:%d res=%d:%d\n",
	 VESA_XRES, VESA_YRES, vbi->bits_per_pixel, vbi->bytes_per_scanline,
	 vbi->mode_attributes, gr_vmem_size >> 10,
	 vbi->red_field_position, vbi->red_mask_size,
	 vbi->green_field_position, vbi->green_mask_size,
	 vbi->blue_field_position, vbi->blue_mask_size,
	 vbi->reserved_field_position, vbi->reserved_mask_size);
  
  switch(VESA_BITS) 
    {
    case 24: 
    case 32:
      break;
    case 15:
    case 16: 
      if (vbi->green_mask_size == 5)
	VESA_BITS = 15;
      else
	VESA_BITS = 16;
      break;
    default: 
      Panic("Video mode with %d bpp not supported!", VESA_BITS); break;
    }

  /* 3) Try to enable native hardware support for graphics card. */
  if (detect_hw(vbe, vbi) < 0)
    {
      /* not known graphics adapter detected -- map video memory */
      l4_addr_t map_addr;
      map_io_mem(vbi->phys_base, gr_vmem_size, 1, "video", &map_addr);
      gr_vmem        = (void*)map_addr;
      gr_vmem_maxmap = gr_vmem + gr_vmem_size;
      vis_vmem       = gr_vmem;
    }

  /* 4) Try to force panning */
  pan_vmem();

  /* 5) Release all memory occupied by int10 emulator */
  x86emu_int10_done();
}
