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

#include <l4/env/mb_info.h>

#include "output.h"

extern void drops_qws_notify(void);

static int    scr_width, scr_height; /* screen dimensions */
static int    scr_depth;             /* color depth */
static int    scr_linelength;        /* bytes per scanline */
static void  *scr_adr;               /* physical screen adress */

static int
vc_map_video_mem(l4_addr_t addr, l4_size_t size,
                 l4_addr_t *vaddr, l4_offs_t *offset) {
	int error;
	l4_addr_t m_addr;
	l4_uint32_t rg;
	l4_uint32_t dummy;
	l4_msgdope_t result;
	l4_threadid_t my_task_preempter_id, my_task_pager_id;

	*offset = addr & ~L4_SUPERPAGEMASK;
	addr   &= L4_SUPERPAGEMASK;
	size    = (size + *offset + L4_SUPERPAGESIZE-1) & L4_SUPERPAGEMASK;

	if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, vaddr, &rg))) {
		LOG("Error %d reserving region size=%dMB for video memory\n",
		 error, size>>20);
		enter_kdebug("map_video_mem");
		return 0;
	}

	/* get region manager's pager */
	my_task_preempter_id = my_task_pager_id = L4_INVALID_ID;
	l4_thread_ex_regs(l4rm_region_mapper_id(), (l4_uint32_t)-1, (l4_uint32_t)-1,
	                  &my_task_preempter_id, &my_task_pager_id,
	                  &dummy, &dummy, &dummy);

	LOG("Mapping video memory at 0x%08x to 0x%08x (size=%dMB)\n",
	       addr, *vaddr, size>>20);

	/* check here for curious video buffer, one candidate is VMware */
	if (addr < 0x80000000) {
		LOG("Video memory address is below 2GB (0x80000000), don't know\n"
		 "how to map it as device super i/o page.\n");
		return -L4_EINVAL;
	}

	for (m_addr=*vaddr; size>0; size-=L4_SUPERPAGESIZE,
	     addr+=L4_SUPERPAGESIZE, m_addr+=L4_SUPERPAGESIZE) {
		for (;;) {
			/* we could get l4_thread_ex_regs'd ... */
			error = l4_ipc_call(my_task_pager_id,
                         L4_IPC_SHORT_MSG, (addr-0x40000000) | 2, 0,
			 L4_IPC_MAPMSG(m_addr, L4_LOG2_SUPERPAGESIZE),
			 &dummy, &dummy,
			 L4_IPC_NEVER, &result);
			if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
				break;
		}
		if (error) {
			LOG("Error 0x%02d mapping video memory\n", error);
			enter_kdebug("map_video_mem");
			return -L4_EINVAL;
		}
		if (!l4_ipc_fpage_received(result)) {
			LOG("No fpage received, result=0x%08x\n", result.msgdope);
			enter_kdebug("map_video_mem");
			return -L4_EINVAL;
		}
	}

	LOG("mapping: vaddr=0x%x size=%d(0x%x) offset=%d(0x%x)\n",
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

