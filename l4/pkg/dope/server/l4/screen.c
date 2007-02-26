/*
 * \brief	DOpE screen module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 * 
 * This component  is the screen  output interface of
 * DOpE. It handles two frame buffers: the physically
 * visible frame buffer and a  background buffer. All
 * graphics  operations are applied to the background
 * buffer and transfered to the physical frame buffer
 * afterwards. This component is also responsible for
 * drawing the mouse cursor.
 */

 
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>
#include <l4/generic_io/libio.h>
  
#include "dope-config.h"
#include "memory.h"
#include "screen.h"
#include "clipping.h"

/* video driver stuff */
#include <l4/oskit10/grub_mb_info.h>
#include <l4/oskit10/grub_vbe_info.h>  
extern struct grub_multiboot_info* _mbi;

/*** VARIABLES OF THE SCREEN MODULE ***/

static s32    scr_width,scr_height;		/* screen dimensions */
static s32    scr_depth;				/* color depth */
static s32    scr_linelength;			/* bytes per scanline */
static void  *scr_adr;					/* physical screen adress */
static void  *buf_adr;					/* adress of screen buffer (doublebuffering) */
static s32    curr_mx=100,curr_my=100;	/* current mouse cursor position */
static s32    cursor_state=1;			/* mouse cursor on/off */
static s32    dax1,day1,dax2,day2;		/* current draw area */
static s32    mouse_visible=0;			/* visibility state of mouse cursor */

static struct clipping_services *clip;
static struct memory_services 	*mem;

/*** PUBLIC ***/

int init_screen(struct dope_services *d);
int screen_use_l4io = 0;  /* whether to use l4io server or not, default no */


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/


static int dope_l4io_init(void) {
	l4io_info_t *io_info_addr = (l4io_info_t*)-1;
  
	if (l4io_init(&io_info_addr, L4IO_DRV_INVALID)) {
		Panic("Couldn't connect to L4 IO server!");
		return 1;
	}  
	return 0;
}


static int
vc_map_video_mem(l4_addr_t addr, l4_size_t size,
		 l4_addr_t *vaddr, l4_offs_t *offset)
{
  int error;
  l4_addr_t m_addr;
  l4_uint32_t rg;
  l4_uint32_t dummy;
  l4_msgdope_t result;
  l4_threadid_t my_task_preempter_id, my_task_pager_id;

  if (!screen_use_l4io) {
    *offset = addr & ~L4_SUPERPAGEMASK;
    addr &= L4_SUPERPAGEMASK;
    size   = (size + *offset + L4_SUPERPAGESIZE-1) & L4_SUPERPAGEMASK;

    if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, vaddr, &rg)))
      {
	printf("Error %d reserving region size=%dMB for video memory\n", 
		error, size>>20);
	enter_kdebug("map_video_mem");
	return 0;
      }

    /* get region manager's pager */
    my_task_preempter_id = my_task_pager_id = L4_INVALID_ID;
    l4_thread_ex_regs(l4rm_region_mapper_id(), (l4_uint32_t)-1, (l4_uint32_t)-1,
		      &my_task_preempter_id, &my_task_pager_id, 
		      &dummy, &dummy, &dummy);
    
    printf("Mapping video memory at 0x%08x to 0x%08x (size=%dMB)\n",
	   addr, *vaddr, size>>20);

    /* check here for curious video buffer, one candidate is VMware */
    if (addr < 0x80000000)
      {
	printf("Video memory address is below 2GB (0x80000000), don't know\n"
	       "how to map it as device super i/o page.\n");
	return -L4_EINVAL;
      }

    for (m_addr=*vaddr; size>0; size-=L4_SUPERPAGESIZE,
				addr+=L4_SUPERPAGESIZE, 
				m_addr+=L4_SUPERPAGESIZE)
      {
	for (;;)
	  {
	    /* we could get l4_thread_ex_regs'd ... */
	    error = l4_i386_ipc_call(my_task_pager_id,
				     L4_IPC_SHORT_MSG, (addr-0x40000000) | 2, 0,
				     L4_IPC_MAPMSG(m_addr, L4_LOG2_SUPERPAGESIZE),
				       &dummy, &dummy,
				     L4_IPC_NEVER, &result);
	    if (error != L4_IPC_SECANCELED && error != L4_IPC_SEABORTED)
	      break;
	  }
	if (error)
	  {
	    printf("Error 0x%02d mapping video memory\n", error);
	    enter_kdebug("map_video_mem");
	    return -L4_EINVAL;
	  }
	if (!l4_ipc_fpage_received(result))
	  {
	    printf("No fpage received, result=0x%08x\n", result.msgdope);
	    enter_kdebug("map_video_mem");
	    return -L4_EINVAL;
	  }
      }
    }
  else
    {
      printf("dope: addr=%x size=%dKiB\n", addr, size >> 10);
      if ((*vaddr = l4io_request_mem_region(addr, size, offset)) == 0)
	Panic("Can't request memory region from l4io.");

      printf("Mapped video memory at %08x to %08x+%06x [%dkB] via L4IO\n",
	  addr, *vaddr, *offset, size >> 10);
    }

  printf("mapping: vaddr=%x size=%d(0x%x) offset=%d(0x%x)\n",
      *vaddr, size, size, *offset, *offset);

  return 0;
}


/*** SET UP SCREEN ***/
static long set_screen(long width, long height, long depth) {

	struct vbe_controller *vbe;
	struct vbe_mode *vbi;
	struct grub_multiboot_info *mbi = _mbi;

	l4_addr_t gr_vbase;
	l4_offs_t gr_voffs;

	if (!(mbi->flags & MB_INFO_VIDEO_INFO) || !(mbi->vbe_mode_info)) {
		printf("Did not found VBE info block in multiboot info. "
			   "Perhaps you have\n"
	           "to upgrade GRUB, RMGR or oskit10_support. GRUB "
	           "has to set the \n"
	           "video mode with the vbeset command.\n");
		return 0;
	}
     
	vbe = (struct vbe_controller*) mbi->vbe_control_info;
	vbi = (struct vbe_mode*) mbi->vbe_mode_info;

	vc_map_video_mem(vbi->phys_base, 64*1024*vbe->total_memory,
                     &gr_vbase,&gr_voffs);

	gr_voffs = 0;

	printf("Frame buffer base:  %p\n"
	       "Resolution:         %dx%dx%d\n"
	       "Bytes per scanline: %d\n", (void *)(gr_vbase + gr_voffs),
	       (unsigned)vbi->x_resolution, (unsigned)vbi->y_resolution, 
	       (unsigned)vbi->bits_per_pixel, (unsigned)vbi->bytes_per_scanline );
	       

	scr_adr			= (void *)(gr_vbase + gr_voffs);
	scr_height		= vbi->y_resolution;
	scr_width		= vbi->x_resolution;
	scr_depth		= vbi->bits_per_pixel;
	scr_linelength	        = scr_width; //vbi->bytes_per_scanline/2;

	if (scr_depth!=16) {
	    enter_kdebug("screendepth");
	    return 0;
	}
  
	/* get memory for the screen buffer */
	buf_adr = mem->alloc((scr_height+16)*scr_width*2);
	if (!buf_adr) {
		DOPEDEBUG(printf("Screen(set_screen): out of memory!\n");)
		return 0;
	}

	printf("Current video mode is %dx%d "
         "red=%d:%d green=%d:%d blue=%d:%d res=%d:%d\n",
         vbi->x_resolution, vbi->y_resolution,
         vbi->red_field_position, vbi->red_mask_size,
         vbi->green_field_position, vbi->green_mask_size,
         vbi->blue_field_position, vbi->blue_mask_size,  
         vbi->reserved_field_position, vbi->reserved_mask_size);

	return 1;
}


/*** DEINITIALISATION ***/
static void restore_screen(void) {
	/* nothing to restore - sdl is too good */
	if (buf_adr) mem->free(buf_adr);
	buf_adr=NULL;
}


/*** PROVIDE INFORMATION ABOUT THE SCREEN ***/
static long  get_scr_width	(void)	{return scr_width;}
static long  get_scr_height	(void)	{return scr_height;}
static long  get_scr_depth	(void)	{return scr_depth;}
static void *get_scr_adr	(void)	{return scr_adr;}
static void *get_buf_adr	(void)	{return buf_adr;}


/*** MAKE CHANGES ON THE SCREEN VISIBLE (BUFFERED OUTPUT) ***/
static void update_area_16(long x1,long y1,long x2,long y2) {
	long dx;
	long dy;
	long v;
	long i,j;
	u16 *src,*dst;
	u32 *s,*d;
	

	/* apply clipping to specified area */
	if (x1<(v=clip->get_x1())) x1=v;
	if (y1<(v=clip->get_y1())) y1=v;	
	if (x2>(v=clip->get_x2())) x2=v;
	if (y2>(v=clip->get_y2())) y2=v;
	
	dx=x2-x1;
	dy=y2-y1;
	if (dx<0) return;
	if (dy<0) return;
	
	/* determine offset of left top corner of the area to update */
	src = (u16 *)buf_adr + y1*scr_width + x1;
	dst = (u16 *)scr_adr + y1*scr_linelength + x1;
	
	src = (u16 *)((adr)src & 0xfffffffc);
	dst = (u16 *)((adr)dst & 0xfffffffc);
	dx = (dx>>1) + 1;
	
	for (j=dy+1;j--;) {
		
		/* copy line */
		d=(u32 *)dst;s=(u32 *)src;
		for (i=dx+1;i--;) *(d++)=*(s++);
//		memcpy(dst,src,dx*2);
	
		src+=scr_width;
		dst+=scr_linelength;
	}
}



static void draw_cursor(short *data,long x,long y) {
	static short i,j;
	short *dst= (short *)buf_adr + y*scr_linelength + x;
	short *d;
	short *s=data;
	short w=*(data++),h=*(data++);
	short linelen=w;
	if (x>=scr_width) return;
	if (y>scr_height-16) return;
	if (x>scr_width-16) linelen=scr_width-x;
	if (y>scr_height-16-16) h=scr_height-16-y;
	for (j=0;j<h;j++) {
		d=dst;s=data;
		for (i=0;i<linelen;i++) {
			if (*s) *d=*s;
			d++;s++;
		}
		dst+=scr_linelength;
		data+=w;
	}
}

static short bg_buffer[16][16];

static void save_background(long x,long y) {
	short *src=(short *)buf_adr + y*scr_linelength + x;
	short *dst=(short *)&bg_buffer;
	short *s;
	static int i,j;
	short h=16;
	if (y>scr_height-16) h=scr_height-y;
	for (j=0;j<h;j++) {
		s=src;
		for (i=0;i<16;i++) {
			*(dst++)=*(s++);
		}
		src+=scr_linelength;
	}
}


static void restore_background(long x,long y) {
	short *src=(short *)&bg_buffer;
	short *dst=(short *)buf_adr + y*scr_linelength + x;
	short *d;
	static int i,j;
	short h=16;
	if (y>scr_height-16) h=scr_height-y;
	for (j=0;j<h;j++) {
		d=dst;
		for (i=0;i<16;i++) {
			*(d++)=*(src++);
		}
		dst+=scr_linelength;
	}
}


extern short smallmouse_trp;
extern short bigmouse_trp;


/*** SET MOUSE CURSOR TO THE SPECIFIED POSITION ***/
static void set_mouse_pos(long mx,long my) {
	if (cursor_state==1) {
		if (mouse_visible) {
			restore_background(curr_mx,curr_my);
			mouse_visible=0;
		}
		if ((mx+16>=dax1) && (mx<=dax2) && (my+16>=day1) && (my<=day2)) {
		/* mouse is inside the bad area */
		} else {
			if (!mouse_visible) {
				save_background(mx,my);
				draw_cursor(&bigmouse_trp,mx,my);
				mouse_visible=1;
			}
		}
	}
	update_area_16(mx,my,mx+15,my+15);
	update_area_16(curr_mx,curr_my,curr_mx+15,curr_my+15);
	curr_mx=mx;
	curr_my=my;
}


/*** SET NEW MOUSE SHAPE ***/
static void set_mouse_shape(void *new_shape) {
	/* not built in yet... */
}


/*** SWITCH MOUSE CURSOR OFF (NEEDED FOR DIRECT DRAWING WITH SOFTWARE CURSOR) ***/
static void mouse_off(void) {
	if (cursor_state==1) {
		if (mouse_visible) {
			restore_background(curr_mx,curr_my);
		}
		mouse_visible=0;
	}
	cursor_state--;
}


/*** SWITCH MOUSE CURSOR ON ***/
static void mouse_on(void) {
	cursor_state++;
	if (cursor_state==1) {
		if (!mouse_visible) {
			save_background(curr_mx,curr_my);
			draw_cursor(&bigmouse_trp,curr_mx,curr_my);
			mouse_visible=1;
		}
	}
}


/*** TELL US THE CURRENT DRAW AREA - SO WE CAN REMOVE THE MOUSE IF NEEDED ***/
static void set_draw_area(long x1,long y1,long x2,long y2) {
	dax1=x1;day1=y1;dax2=x2;day2=y2;
	if ((curr_mx+16>=dax1) && (curr_mx<=dax2) && (curr_my+16>=day1) && (curr_my<=day2)) {
		if (mouse_visible) restore_background(curr_mx,curr_my);
		mouse_visible=0;
	} else {
		if (!mouse_visible) {
			save_background(curr_mx,curr_my);
			draw_cursor(&bigmouse_trp,curr_mx,curr_my);
			mouse_visible=1;
		}
	}
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct screen_services services = {
	set_screen:         set_screen,
	restore_screen:     restore_screen,
	get_scr_width:      get_scr_width,
	get_scr_height:     get_scr_height,
	get_scr_depth:      get_scr_depth,
	get_scr_adr:        get_scr_adr,
	get_buf_adr:        get_buf_adr,
	update_area:        update_area_16,
	set_mouse_pos:      set_mouse_pos,
	set_mouse_shape:    set_mouse_shape,
	mouse_off:          mouse_off,
	mouse_on:           mouse_on,
	set_draw_area:      set_draw_area
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_screen(struct dope_services *d) {

	if (screen_use_l4io)
	  dope_l4io_init();
	
	clip 	= d->get_module("Clipping 1.0");
	mem		= d->get_module("Memory 1.0");
	
	d->register_module("Screen 1.0",&services);
	return 1;
}
