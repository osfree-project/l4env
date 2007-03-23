/*
 * \brief   Nitpicker screen initialization
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2007  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/env/errno.h>
#include <l4/env/mb_info.h>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sigma0/sigma0.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>
#include <l4/generic_io/libio.h>

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"

int    scr_width, scr_height;  /* screen dimensions      */
int    scr_depth;              /* color depth            */
int    scr_llen;               /* bytes per scanline     */
void  *scr_adr;                /* physical screen adress */

extern int config_use_l4io;    /* defined in startup.c   */


/*** MAP VIDEO MEMORY INTO LOCAL ADDRESS SPACE ***/
static inline int
vc_map_video_mem(l4_addr_t paddr, l4_size_t size,
                 l4_addr_t *vaddr, l4_offs_t *offset) {
	l4_uint32_t rg;
	l4_threadid_t my_task_pager_id;

	if (!config_use_l4io) {
		*offset = paddr & ~L4_SUPERPAGEMASK;
		paddr  &= L4_SUPERPAGEMASK;
		size    = l4_round_superpage(size + *offset);

		TRY(l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, vaddr, &rg),
		    "Reserving region size=%ldMB for video memory failed.", (unsigned long)size>>20);

		/* get region manager's pager */
		my_task_pager_id = l4_thread_ex_regs_pager(l4rm_region_mapper_id());

		switch (l4sigma0_map_iomem(my_task_pager_id, paddr, *vaddr, size, 1)) {
			case -2:
				printf("IPC error mapping video memory\n");
				enter_kdebug("map_video_mem");
				return -L4_EINVAL;
			case -3:
				printf("No fpage received\n");
				enter_kdebug("map_video_mem");
				return -L4_EINVAL;
      			case -4:
	    			printf("Video memory address is below 2GB (0x80000000), don't know\n"
		 		    "how to map it as device super i/o page.\n");
			     	return -L4_EINVAL;
		}
	} else {
		*offset = 0;
		if ((*vaddr = l4io_request_mem_region(paddr, size, L4IO_MEM_WRITE_COMBINED)) == 0)
			Panic("Can't request memory region from l4io.");
	}

	return 0;
}


/*** SET UP SCREEN ***/
int scr_init(void) {

	l4util_mb_vbe_ctrl_t *vbe;
	l4util_mb_vbe_mode_t *vbi;
	l4util_mb_info_t *mbi = l4env_multiboot_info;

	l4_addr_t gr_vbase;
	l4_offs_t gr_voffs;

	if (!(mbi->flags & L4UTIL_MB_VIDEO_INFO) || !(mbi->vbe_mode_info)) {
		printf("Did not find VBE info block in multiboot info. "
		       "GRUB has to set the \n"
		       "video mode with the vbeset command.\n");
		return -1;
	}

	vbe = (l4util_mb_vbe_ctrl_t*) mbi->vbe_ctrl_info;
	vbi = (l4util_mb_vbe_mode_t*) mbi->vbe_mode_info;

	vc_map_video_mem((vbi->phys_base >> L4_SUPERPAGESHIFT) << L4_SUPERPAGESHIFT,
	                 64*1024*vbe->total_memory, &gr_vbase, &gr_voffs);

	TRY(!gr_vbase, "Can't request memory region from l4io.\n");

	gr_voffs += vbi->phys_base & ((1<< L4_SUPERPAGESHIFT)-1);

	printf("Frame buffer base:  %p\n"
	       "Resolution:         %dx%dx%d\n"
	       "Bytes per scanline: %d\n", (void *)(gr_vbase + gr_voffs),
	       vbi->x_resolution, vbi->y_resolution,
	       vbi->bits_per_pixel, vbi->bytes_per_scanline );

	scr_adr    = (void *)(gr_vbase + gr_voffs);
	scr_height = vbi->y_resolution;
	scr_width  = vbi->x_resolution;
	scr_depth  = vbi->bits_per_pixel;
	scr_llen   = vbi->bytes_per_scanline / (scr_depth/8);

	printf("Current video mode is %dx%d "
	       "red=%d:%d green=%d:%d blue=%d:%d res=%d:%d\n",
	       vbi->x_resolution, vbi->y_resolution,
	       vbi->red_field_position, vbi->red_mask_size,
	       vbi->green_field_position, vbi->green_mask_size,
	       vbi->blue_field_position, vbi->blue_mask_size,
	       vbi->reserved_field_position, vbi->reserved_mask_size);

	return 0;
}

