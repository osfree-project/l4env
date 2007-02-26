/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/drivers/output_sdl.cc
 * \brief  VESA framebuffer driver for Qt/Embedded.
 *
 * \date   10/24/2004
 * \author Josef Spillner <js177634@inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2005 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*
 * Based on DOpE's l4/scrdrv.c
 */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/sigma0/sigma0.h>

#include <l4/env/mb_info.h>

#include "output.h"

extern void drops_qws_notify(void);

static int    scr_width, scr_height; /* screen dimensions */
static int    scr_depth;             /* color depth */
static int    scr_linelength;        /* bytes per scanline */
static void  *scr_adr;               /* physical screen adress */

static int
vc_map_video_mem(l4_addr_t paddr, l4_size_t size,
                 l4_addr_t *vaddr, l4_offs_t *offset) {
	int error;
	l4_uint32_t rg;
	l4_msgdope_t result;
	l4_threadid_t my_task_pager_id;

	*offset = paddr & ~L4_SUPERPAGEMASK;
	paddr   = l4_trunc_superpage(paddr);
	size    = l4_round_superpage(size + *offset);

	if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, vaddr, &rg))) {
		LOG("Error %d reserving region size=%dMB for video memory\n",
		 error, size>>20);
		enter_kdebug("map_video_mem");
		return 0;
	}

	/* get region manager's pager */
	my_task_pager_id = l4_thread_ex_regs_pager(l4rm_region_mapper_id());

	LOG("Mapping video memory at 0x%08lx to 0x%08lx (size=%dMB)\n",
	       paddr, *vaddr, size>>20);

	/* check here for curious video buffer, one candidate is VMware */
	switch (l4sigma0_map_iomem(my_task_pager_id, paddr, *vaddr, size, 1)) {
		case -2:
			LOG("Error 0x%02d mapping video memory\n", error);
			enter_kdebug("map_video_mem");
			return -L4_EINVAL;
		case -3:
			LOG("No fpage received, result=0x%08lx\n", result.msgdope);
			enter_kdebug("map_video_mem");
			return -L4_EINVAL;
		case -4:
			LOG("Video memory address is below 2GB (0x80000000), don't know\n"
			    "how to map it as device super i/o page.\n");
			return -L4_EINVAL;
	}

	LOG("mapping: vaddr=0x%lx size=%d(0x%x) offset=%ld(0x%lx)\n",
	       *vaddr, size, size, *offset, *offset);

	return 0;
}


long drops_qws_set_screen(long width, long height, long depth) {
	l4util_mb_vbe_ctrl_t *vbe;
	l4util_mb_vbe_mode_t *vbi;
	l4util_mb_info_t *mbi = l4env_multiboot_info;

	l4_addr_t gr_vbase;
	l4_offs_t gr_voffs;

	if (!(mbi->flags & L4UTIL_MB_VIDEO_INFO) || !(mbi->vbe_mode_info)) {
		LOG("Did not found VBE info block in multiboot info. "
		       "Perhaps you have\n"
		       "to upgrade GRUB, RMGR or oskit10_support. GRUB "
		       "has to set the \n"
		       "video mode with the vbeset command.\n");
		enter_kdebug("PANIC");
		return 0;
	}

	vbe = (l4util_mb_vbe_ctrl_t*) mbi->vbe_ctrl_info;
	vbi = (l4util_mb_vbe_mode_t*) mbi->vbe_mode_info;

	vc_map_video_mem((vbi->phys_base >> L4_SUPERPAGESHIFT) << L4_SUPERPAGESHIFT,
                         64*1024*vbe->total_memory,
	                 &gr_vbase,&gr_voffs);

        gr_voffs += vbi->phys_base & ((1<< L4_SUPERPAGESHIFT)-1);

	gr_voffs = 0;

	LOG("Frame buffer base:  %p\n"
	       "Resolution:         %dx%dx%d\n"
	       "Bytes per scanline: %d\n", (void *)(gr_vbase + gr_voffs),
	       vbi->x_resolution, vbi->y_resolution,
	       vbi->bits_per_pixel, vbi->bytes_per_scanline );

	scr_adr         = (void *)(gr_vbase + gr_voffs);
	scr_height      = vbi->y_resolution;
	scr_width       = vbi->x_resolution;
	scr_depth       = vbi->bits_per_pixel;
	scr_linelength  = vbi->bytes_per_scanline;

	LOG("Current video mode is %dx%d "
	       "red=%d:%d green=%d:%d blue=%d:%d res=%d:%d\n",
	       vbi->x_resolution, vbi->y_resolution,
	       vbi->red_field_position, vbi->red_mask_size,
	       vbi->green_field_position, vbi->green_mask_size,
	       vbi->blue_field_position, vbi->blue_mask_size,
	       vbi->reserved_field_position, vbi->reserved_mask_size);

	return 1;
}

long  drops_qws_get_scr_width  (void) {return scr_width;}
long  drops_qws_get_scr_height (void) {return scr_height;}
long  drops_qws_get_scr_depth  (void) {return scr_depth;}
void *drops_qws_get_scr_adr    (void) {return scr_adr;}
long  drops_qws_get_scr_line   (void) {return scr_linelength;}

void  drops_qws_refresh_screen (void) {drops_qws_notify();}

