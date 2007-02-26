/* $Id$ */

/** \file	con/server/src/vc.c
 *
 *  \brief	virtual console stuff
 *		ATTENTION: it's multi threaded
 */

/* L4 includes */
#include <l4/con/l4con.h>
#include <l4/l4rm/l4rm.h>
#include <l4/sys/syscalls.h>
#include <l4/con/l4contxt.h>
#include <l4/env/errno.h>
#include <l4/util/bitops.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/generic_io/libio.h>

/* OSKit includes */
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "vc.h"
#include "dsi_vc.h"
#include "ipc.h"
#include "pslim_func.h"
#include "con_macros.h"
#include "con_config.h"
#include "con-server.h"
#include "con_hw/init.h"
#include "con_hw/iomem.h"
#include "con_hw/fourcc.h"
#include "con_hw/vidix.h"
#include "con_yuv2rgb/yuv2rgb.h"

/* video driver stuff */
#include <l4/oskit10/grub_mb_info.h>
#include <l4/oskit10/grub_vbe_info.h>
extern struct grub_multiboot_info* _mbi;

void *gr_vmem;		/**< linear video framebuffer */
void *vis_vmem;		/**< visible video framebuffer */

/* in main.c */
extern int fg_vc, want_vc, update_id;
extern l4lock_t want_vc_lock;
extern int noaccel;
extern int pan;
extern int use_l4io;

extern const char _binary_font_psf_start[];
static unsigned accel_caps  = 0;
static unsigned status_area = 0;

static int cscs_stream_active = 0;
static vidix_video_eq_t cscs_eq;

static int vc_open_in(struct l4con_vc*);
static int vc_open_out(struct l4con_vc*);
static int vc_font_init(void);
static int vc_l4io_init(void);

/* dummy function for sync, does nothing */
static void _nothing_sync(void)
{
}

con_accel_t hw_accel =
{ copy: sw_copy, fill:sw_fill, sync:_nothing_sync };

pslim_copy_fn fg_do_copy = sw_copy;
pslim_copy_fn bg_do_copy = sw_copy;
pslim_fill_fn fg_do_fill = sw_fill;
pslim_fill_fn bg_do_fill = sw_fill;
pslim_sync_fn fg_do_sync = _nothing_sync;
pslim_sync_fn bg_do_sync = _nothing_sync;
pslim_drty_fn fg_do_drty = 0;

/** color tables for puts_attr */
static const pslim_color_t color_tab15[16] =
{ 
  0x0000, 0x0015, 0x02a0, 0x02b5, 0x5400, 0x5415, 0x5540, 0x56b5,
  0x294a, 0x295f, 0x2bea, 0x2bff, 0x7d4a, 0x7d5f, 0x7fea, 0x7fff
};
static const pslim_color_t color_tab16[16] =
{ 
  0x0000, 0x0015, 0x0540, 0x0555, 0xa800, 0xa815, 0xaaa0, 0xad55,
  0x52aa, 0x52bf, 0x57ea, 0x57ff, 0xfaaa, 0xfabf, 0xffea, 0xffff
};
static const pslim_color_t color_tab32[16] =
{
  0x00000000, 0x000000aa, 0x0000aa00, 0x0000aaaa,
  0x00aa0000, 0x00aa00aa, 0x00aa5500, 0x00aaaaaa,
  0x00555555, 0x005555ff, 0x0055ff55, 0x0055ffff,
  0x00ff5555, 0x00ff55ff, 0x00ffff55, 0x00ffffff
};

extern l4_uint16_t VESA_XRES;
extern l4_uint16_t VESA_YRES;
extern l4_uint8_t VESA_BITS;
extern l4_uint16_t VESA_BPL;
       l4_uint8_t VESA_RES;
extern l4_uint8_t FONT_XRES;
extern l4_uint8_t FONT_YRES;
extern struct l4con_vc *vc[];

static void vc_init_gr(void);
static l4_uint32_t pan_offs_x, pan_offs_y;

/** convert pslim_color_t into ``drawable'' color */
static inline void
convert_color(struct l4con_vc *vc, pslim_color_t *color)
{ 
  /* if the highest bit is 1, don't convert the color */
  if ((*color & 0x80000000) == 0)
    {
      switch (vc->gmode & GRAPH_BPPMASK)
	{
	case GRAPH_BPP_24:
	case GRAPH_BPP_32:
	  *color &= 0x00FFFFFF;
	  break;
	case GRAPH_BPP_15:
	  *color = ((*color & 0x00F80000) >> 9)
		 | ((*color & 0x0000F800) >> 6)
		 | ((*color & 0x000000F8) >> 3);
	  break;
	case GRAPH_BPP_16:
	default:
	  *color = ((*color & 0x00F80000) >> 8)
		 | ((*color & 0x0000FC00) >> 5)
		 | ((*color & 0x000000F8) >> 3);
	  break;
	}
    }
}


void 
vc_init()
{
  int currvc;

  if (use_l4io)
    vc_l4io_init();
  
  vc_font_init();
  
  vc_init_gr();

  for (currvc = 0; currvc < MAX_NR_L4CONS; currvc++) 
    {
      /* malloc */
      vc[currvc] = (struct l4con_vc *) malloc(sizeof(struct l4con_vc));
      ASSERT(vc[currvc]);
      
      /* init values */
      vc[currvc]->vc_number = currvc;
      vc[currvc]->mode = CON_CLOSED;
      vc[currvc]->dsi_mode = CON_NOT_DSI;
      vc[currvc]->vfb = 0;
      /* need no lock/unlock here because we're 
       * not multithreaded yet */
      vc[currvc]->fb_lock = L4LOCK_UNLOCKED;
    }

  /* the master console is special. It is shown if no other vc's are open */
  vc[0]->mode            = CON_MASTER | CON_OUT;
  vc[0]->vfb             = 0;
  vc[0]->vfb_used        = 0;
  vc[0]->fb              = gr_vmem;
  vc[0]->fb_mapped       = 0;
  vc[0]->vc_partner_l4id = L4_INVALID_ID;
  vc[0]->vc_l4id         = L4_INVALID_ID;
  vc[0]->ev_partner_l4id = L4_NIL_ID;
  vc[0]->gmode           = VESA_RES;
  vc[0]->xres            = VESA_XRES;
  vc[0]->xofs            = pan_offs_x;
  vc[0]->yofs            = pan_offs_y;
  vc[0]->user_xofs       = 0;
  vc[0]->user_yofs       = 0;
  vc[0]->user_xres       = VESA_XRES;
  vc[0]->user_yres       = VESA_YRES - status_area;
  vc[0]->yres            = VESA_YRES;
  vc[0]->bpp             = VESA_BITS;
  vc[0]->bytes_per_pixel = (VESA_BITS+7)/8;
  vc[0]->bytes_per_line  = VESA_BPL;
  vc[0]->flags           = accel_caps;
  vc[0]->do_sync         = fg_do_sync;
  vc[0]->do_drty         = fg_do_drty;
}

static int
vc_l4io_init(void)
{
  l4io_info_t *io_info_addr = (l4io_info_t*)-1;

  if (l4io_init(&io_info_addr, L4IO_DRV_INVALID))
    {
      PANIC("Couldn't connect to L4 IO server!");
      return 1;
    }

  return 0;
}

static int
vc_detect_hw(struct vbe_controller *vbe, struct vbe_mode *vbi)
{
  if (!noaccel)
    {
      l4_addr_t vid_mem_addr = vbi->phys_base;
      l4_size_t vid_mem_size = 64*1024*vbe->total_memory;

      if (con_hw_init(VESA_XRES, VESA_YRES, &VESA_BITS,
		      vid_mem_addr, vid_mem_size, use_l4io,
		      &hw_accel, (l4_addr_t*)&gr_vmem)<0)
	{
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

      vis_vmem = gr_vmem;

      if (pan && hw_accel.pan)
	{
	  unsigned next_super_offs, start;

	  /* hardware support for panning the display */
	  next_super_offs = L4_SUPERPAGESIZE;

	  /* consider overflow on memory modes with more than 4MB */
	  if (vbi->y_resolution * vbi->bytes_per_scanline > next_super_offs)
	    next_super_offs += L4_SUPERPAGESIZE;

	  if (next_super_offs + status_area*vbi->bytes_per_scanline
		      > vid_mem_size)
	    {
	      printf("WARNING: Can't pan display: Only have %dkB video memory "
		     "need %dkB\n", 
		     vid_mem_size / 1024, 
		     (next_super_offs + status_area*vbi->bytes_per_scanline) 
		     / 1024);
	    }
	  else
	    {
	      unsigned x, y;
	      unsigned bpp = (vbi->bits_per_pixel + 1) / 8;
	      
	      x = (next_super_offs % vbi->bytes_per_scanline) / bpp;
	      y = (next_super_offs / vbi->bytes_per_scanline)
		- vbi->y_resolution + status_area;

	      /* pan graphics card */
	      hw_accel.pan(&x, &y);
	      
	      pan_offs_x = x;
	      pan_offs_y = y;
	      
	      start = y * vbi->bytes_per_scanline + x * bpp;
	      vis_vmem  = gr_vmem + start;
	      
	      printf("Display panned to %d:%d\n", y, x);
	    }
	}

      return 0;
    }

  return -L4_ENOTFOUND;
}

/* init VESA console driver */
static void 
vc_init_gr(void)
{
  struct vbe_mode *vbi;
  struct vbe_controller *vbe;
  struct grub_multiboot_info *mbi = _mbi;
  l4_size_t vid_mem_size;

  if (!(mbi->flags & MB_INFO_VIDEO_INFO) ||
      !(mbi->vbe_mode_info))
    {
      PANIC(           "Did not found VBE info block in multiboot info.\n"
	    "Perhaps you have to upgrade GRUB, RMGR or oskit10_support.\n"
	    "GRUB has to set the video mode with the vbeset command.");
    }
  
  vbe = (struct vbe_controller*) mbi->vbe_control_info;
  vbi = (struct vbe_mode*) mbi->vbe_mode_info;

  vid_mem_size = 64*1024*vbe->total_memory;
  
  VESA_YRES = vbi->y_resolution;
  VESA_XRES = vbi->x_resolution;
  VESA_BITS = vbi->bits_per_pixel;
  VESA_BPL  = vbi->bytes_per_scanline;
  VESA_RES  = 0;

  printf("VESA reports %dx%d@%d (%04x) "
         "red=%d:%d green=%d:%d blue=%d:%d res=%d:%d\n",
	 VESA_XRES, VESA_YRES, vbi->bits_per_pixel, vbi->mode_attributes,
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
      PANIC("Video mode with %d bpp not supported!", VESA_BITS); break;
    }
  
  if (vc_detect_hw(vbe, vbi)<0)
    /* not known graphics adapter detected -- map video memory */
    map_io_mem(vbi->phys_base, vid_mem_size, "video", (l4_addr_t*)&gr_vmem);

  vis_vmem = gr_vmem;

  switch(VESA_XRES)
    {
    case 800:  VESA_RES = GRAPH_RES_800;  break;
    case 1024: VESA_RES = GRAPH_RES_1024; break;
    case 1152: VESA_RES = GRAPH_RES_1152; break;
    case 1280: VESA_RES = GRAPH_RES_1280; break;
    case 1600: VESA_RES = GRAPH_RES_1600; break;
    case 640:
    default:   VESA_RES = GRAPH_RES_640;
    }
  
  switch(VESA_BITS) 
    {
    case 15: VESA_RES |= GRAPH_BPP_15; break;
    case 16: VESA_RES |= GRAPH_BPP_16; break;
    case 24: VESA_RES |= GRAPH_BPP_24; break;
    case 32: VESA_RES |= GRAPH_BPP_32; break;
    }

  yuv2rgb_init(VESA_BITS, MODE_RGB);
}

/******************************************************************************
 * implementation of service functions                                        *
 *****************************************************************************/

/******************************************************************************
 * open_vc                                                                    *
 *                                                                            *
 * param: vc          ... current information structure                       *
 *        mode        ... CON_OUT, CON_INOUT                                  *
 * ret:   0           ... success                                             *
 *                                                                            *
 * open new virtual console (already set to CON_OPENING)                      *
 *****************************************************************************/
int
vc_open(struct l4con_vc *vc, l4_uint32_t mode)
{
  int error;
	
  switch (mode) 
    {
    case CON_INOUT:
      
      /* IN specific */
      if ((error = vc_open_in(vc)) ) 
	return error;

      /* OUT specific */
      if ((error = vc_open_out(vc))) 
	return error;

      /* switch to new opened vc */
      vc->mode = CON_INOUT;
		
      l4lock_lock(&want_vc_lock);
      want_vc = vc->vc_number;
      l4lock_unlock(&want_vc_lock);
      break;
      
    case CON_OUT:
      if ((error = vc_open_out(vc)))
	return error;
		
      /* switch to new opened vc */
      vc->mode = CON_OUT;
      
      l4lock_lock(&want_vc_lock);
      want_vc = vc->vc_number;
      l4lock_unlock(&want_vc_lock);
      break;
    }
  
  return 0;
}

static int
vc_font_init(void)
{
  unsigned chars;
  
  /* all consoles can use puts interface */
  /* check magic number of .psf */
  if ((_binary_font_psf_start[0] != 0x36) ||
      (_binary_font_psf_start[1] != 0x04))
    PANIC("psf magic number failed");
  
  FONT_XRES = 8;
  FONT_YRES = _binary_font_psf_start[3];
  
  /* check file mode */
  switch (_binary_font_psf_start[2]) 
    {
    case 0:
    case 2:
      chars = 256;
      break;
    case 1:
    case 3:
      chars = 512; 
      break;
    default:
      PANIC("bad psf font file magic %02x!", 
	  _binary_font_psf_start[2]);
    }
  
  printf("Character size is %dx%d, font has %d characters\n", 
	 FONT_XRES, FONT_YRES, chars);
  
  status_area = FONT_YRES + 4;
  
  return 0;
}

/******************************************************************************
 * open_vc_in                                                                 *
 *                                                                            *
 * param: vc          ... current information structure                       *
 * ret:   0           ... success                                             *
 *        -1          ... failed                                              *
 *                                                                            *
 * do CON_IN part of open                                                     *
 *****************************************************************************/
int 
vc_open_in(struct l4con_vc *vc)
{
  /* is there an event handler thread */
  /* and is he 'alive' */
  if (!thread_exists(vc->ev_partner_l4id)) 
    {
      return -CON_ETHREAD;
    }
  
  return 0;
}

/******************************************************************************
 * open_vc_out                                                                *
 *                                                                            *
 * param: vc          ... current information structure                       *
 * ret:   0           ... success                                             *
 *        otherwise   ... failed                                              *
 *                                                                            *
 * do CON_OUT part of open                                                    *
 *****************************************************************************/
int 
vc_open_out(struct l4con_vc *vc)
{
  /* set default video mode */
  vc->gmode     = VESA_RES;
  vc->user_xofs = 0;
  vc->user_yofs = 0;
  vc->user_xres = VESA_XRES;
  vc->user_yres = VESA_YRES - status_area;
  vc->xres      = VESA_XRES;
  vc->yres      = VESA_YRES;
  vc->xofs      = pan_offs_x;
  vc->yofs      = pan_offs_y;
  vc->bpp       = VESA_BITS;
  
  vc->bytes_per_pixel = (vc->bpp+7)/8;
  vc->bytes_per_line  = VESA_BPL;
  vc->vfb_size        = vc->yres * vc->bytes_per_line;
  vc->flags           = accel_caps;

  vc->do_copy = bg_do_copy;
  vc->do_fill = bg_do_fill;
  vc->do_sync = bg_do_sync;
  vc->do_drty = fg_do_drty;
  
  switch (vc->gmode & GRAPH_BPPMASK)
    {
    case GRAPH_BPP_32:
    case GRAPH_BPP_24: vc->color_tab = color_tab32; break;
    case GRAPH_BPP_15: vc->color_tab = color_tab15; break;
    case GRAPH_BPP_16:
    default:           vc->color_tab = color_tab16; break;
    }
  
  if(vc->vfb_used) 
    {
      int error;
      char ds_name[32];
      l4dm_dataspace_t ds;

      sprintf(ds_name, "vfb for %x.%x", 
	     vc->vc_partner_l4id.id.task, 
	     vc->vc_partner_l4id.id.lthread);
      if ((error = l4dm_mem_open(L4DM_DEFAULT_DSM, vc->vfb_size, 
				 0, 0, ds_name, &ds)))
	{
    	  INFO("Error %d requesting %d bytes for vc\n", error, vc->vfb_size);
	  PANIC("open_vc_out");
	  return -CON_ENOMEM;
	}
      if ((error = l4rm_attach(&ds, vc->vfb_size, 0, L4DM_RW,
			      (void**)&vc->vfb)))
	{
	  INFO("Error %d attaching vc dataspace\n", error);
	  PANIC("open_vc_out");
	}

      memset(vc->vfb, 0, vc->vfb_size);
      vc->fb = vc->vfb;
    }
  
  INFO("vc[%d] %dx%d@%d, gmode:0x%x\n", 
      vc->vc_number,vc->xres, vc->yres,vc->bpp,vc->gmode);
  return 0;
}

/******************************************************************************
 * close_vc                                                                   *
 *                                                                            *
 * param: vc          ... current information structure                       *
 * ret:   0           ... success                                             *
 *                                                                            *
 * close current virtual console                                              *
 *****************************************************************************/
int 
vc_close(struct l4con_vc *vc)
{
  /* XXX notify event thread of partner */
  
  /* temporary mode: no output allowed, but occupied */
  vc->mode = CON_CLOSING;
  vc->dsi_mode = CON_NOT_DSI;
  
  /* deallocate memory */
  if (vc->vfb_used && vc->vfb)
    {
      l4dm_mem_release(vc->vfb);
      vc->vfb = 0;
    }

  vc->fb = 0;

  if (vc->sbuf1)
    {
      l4dm_mem_release(vc->sbuf1);
      vc->sbuf1 = 0;
    }
  if (vc->sbuf2)
    {
      l4dm_mem_release(vc->sbuf2);
      vc->sbuf2 = 0;
    }
  if (vc->sbuf3)
    {
      l4dm_mem_release(vc->sbuf3);
      vc->sbuf3 = 0;
    }
  
  vc->ev_partner_l4id = L4_NIL_ID;
  
  l4lock_lock(&want_vc_lock);
  if (vc->vc_number == fg_vc) 
    want_vc = -1;
  else
    update_id = 1;
  l4lock_unlock(&want_vc_lock);
  
  return 0;
}

/** render string to screen
 * @pre have vc->fb_lock */
static int
vc_puts(struct l4con_vc *vc, int from_user,
	l4_uint8_t *str, int len, l4_int16_t x, l4_int16_t y,
	pslim_color_t fg_color, pslim_color_t bg_color)
{
  int i, j;

  convert_color(vc, &fg_color);
  convert_color(vc, &bg_color);
  
  if(vc->fb != 0)
    {
      for (i=0; i<len; i++, str++)
	{	
	  /* optimization: collect spaces */
	  for (j=0; (i<len) && (*str == ' '); i++, j++, str++)
	    ;	  
	  
	  if (j>0)
	    {
	      pslim_rect_t rect = { x, y, j*FONT_XRES, FONT_YRES };
	      
	      pslim_fill(vc, from_user, &rect, bg_color);
	      x += j*FONT_XRES;
	      i--; str--;
	    }
	  else
	    {
	      pslim_rect_t rect = { x, y, FONT_XRES, FONT_YRES };
	      
	      pslim_bmap(vc, from_user, 
			 &rect, fg_color, bg_color, 
			 (void*) &_binary_font_psf_start[rect.h * (*str)+4],
			 pSLIM_BMAP_START_MSB);
	      x += FONT_XRES;
	    }
	}
    }

  return 0;
}

/** fill rectangle of screen
 * @pre have vc->fb_lock */
static int
vc_fill(struct l4con_vc *vc, int from_user,
	pslim_rect_t *rect, con_pslim_color_t color)
{
  if (!(vc->mode & CON_OUT))
    return -CON_EPERM;

  convert_color(vc, &color);

  if (vc->fb != 0)
    pslim_fill(vc, from_user, rect, color);

  return 0;
}

/** put characters with scale >= 1
 * @pre have vc->fb_lock */
static int
vc_puts_scale(struct l4con_vc *vc, int from_user,
	      l4_uint8_t *str, int len, l4_int16_t x, l4_int16_t y,
  	      pslim_color_t fg_color, pslim_color_t bg_color,
	      int scale_x, int scale_y)
{
  int pix_x, pix_y;
  pslim_rect_t rect = { x, y, FONT_XRES*scale_x, FONT_YRES*scale_y };

  pix_x = scale_x;
  if (scale_x >= 5)
    pix_x = scale_x * 14/15;
  pix_y = scale_y;
  if (scale_y >= 5)
    pix_y = scale_y * 14/15;

  convert_color(vc, &fg_color);
  convert_color(vc, &bg_color);
  
  if(vc->fb != 0)
    {
      int i;
      for (i=0; i<len; i++, str++)
	{
	  pslim_rect_t lrect = { rect.x, rect.y, pix_x, pix_y };
	  const char *bmap = &_binary_font_psf_start[FONT_YRES*(*str) + 4];
	  int j;
	  
	  for (j=0; j<FONT_YRES; j++)
	    {
	      unsigned char mask = 0x80;
	      int k;
	      
	      for (k=0; k<FONT_XRES; k++)
		{
		  pslim_color_t color = (*bmap & mask) ? fg_color : bg_color;
		  pslim_fill(vc, from_user, &lrect, color);
		  lrect.x += scale_x;
		  bmap += (mask &  1);
		  mask  = (mask >> 1) | (mask << 7);
		}
	      lrect.x -= rect.w;
	      lrect.y += scale_y;
	    }
	  rect.x += rect.w;
	}
    }

  return 0;
}

/** show id of current console at bottom of screen
 * @pre have vc->fb_lock */
void
vc_show_id(struct l4con_vc *this_vc)
{
  char id[64];
  int i, x, cnt_vc;
  const pslim_color_t fgc = 0x009999FF;
  const pslim_color_t bgc = 0x00666666;
  pslim_rect_t rect = { 0, this_vc->user_yres,
			this_vc->xres, this_vc->yres-this_vc->user_yres };

  cnt_vc = MAX_NR_L4CONS > 10 ? 9 : MAX_NR_L4CONS-1;

  vc_fill(this_vc, 0, &rect, bgc);

  if (this_vc->vc_number != 0)
    {
      sprintf(id, "DROPS console: partner %x.%02x   ",
    	  this_vc->vc_partner_l4id.id.task,
	  this_vc->vc_partner_l4id.id.lthread);
    }
  else
    {
      strcpy(id, "DROPS console: (all closed)");
    }
  
  vc_puts(this_vc, 0, id, strlen(id),
          2, this_vc->user_yres+2,
          fgc, bgc);

  sprintf(id, "%dx%d@%d%s", 
          this_vc->xres, this_vc->yres, this_vc->bpp,
	  this_vc->fb_mapped ? " [fb mapped]" : "");
  vc_puts(this_vc, 0, id, strlen(id),
	  ((this_vc->xres - strlen(id)*FONT_XRES) / 2), this_vc->user_yres+2,
	  fgc, bgc);

  for (i=1, x=this_vc->xres-cnt_vc*(FONT_XRES+2);
       i<=cnt_vc;
       i++, x+=FONT_XRES+2)
    {
      int _fgc = 0x00000000, _bgc = bgc, tmp;
      
      if (vc[i]->mode == CON_INOUT || vc[i]->mode == CON_OUT)
	_fgc = 0x0000FF00;

      if (this_vc == vc[i])
	{
	  tmp = _fgc; _fgc = _bgc; _bgc = tmp;
	}

      id[0] = '0' + i;

      vc_puts(this_vc, 0, id, 1, x, this_vc->yres-FONT_YRES-2, _fgc, _bgc);
    }
}

/** clear vc
 * @pre have vc->fb_lock */
void
vc_clear(struct l4con_vc *vc)
{
  const pslim_color_t fgc = 0x00223344;
  const pslim_color_t bgc = 0x00000000;
  pslim_rect_t rect = { 0, 0, vc->user_xres, vc->user_yres };
  
  vc_fill(vc, 0, &rect, bgc);

  /* special case is console 0 */
  if (vc->vc_number == 0)
    {
      /* master console, show DROPS label */
      int x, y, scale_x_1, scale_y_1, scale_x_2, scale_y_2;

      scale_x_1 = (vc->user_xres*4/ 5) / (5*FONT_XRES);
      scale_y_1 = (vc->user_yres*6/10) / (1*FONT_YRES);
      x = vc->user_xofs + (vc->user_xres-5*FONT_XRES*scale_x_1)/2;
      y = vc->user_yofs + (vc->user_yres-1*FONT_YRES*scale_y_1)*3/7;
      vc_puts_scale(vc, 0, 
			"DROPS", 5,
		        x, y, fgc, bgc, scale_x_1, scale_y_1);
      scale_x_2 = scale_x_1*10/90;
      scale_y_2 = scale_y_1*10/90;
      x = vc->user_xofs + (vc->user_xres-46*FONT_XRES*scale_x_2)/2;
      y += 1*FONT_YRES*scale_y_1*12/14;
      vc_puts_scale(vc, 0, 
			"The Dresden Real-Time Operating System Project", 46,
    			x, y, fgc, bgc, scale_x_2, scale_y_2);
    }
}


/******************************************************************************
 * con_vc - IDL server functions                                              *
 *****************************************************************************/

/******************************************************************************
 * con_vc_server_smode                                                        *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      mode          ... CON_OUT, CON_INOUT                                  *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Setup mode of current virtual console: input, output or in/out             *
 *****************************************************************************/
l4_int32_t 
con_vc_server_smode(sm_request_t *request, 
		    l4_uint8_t mode, 
		    const con_threadid_t *ev_handler,
		    sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);
  
  if (vc->mode == CON_OPENING)
    {
      /* inital state */
      vc->ev_partner_l4id.lh.low = ev_handler->low;
      vc->ev_partner_l4id.lh.high = ev_handler->high;
      return vc_open(vc, mode);
    }
  else 
    { 
      /* set new event handler */
      vc->ev_partner_l4id.lh.low = ev_handler->low;
      vc->ev_partner_l4id.lh.high = ev_handler->high;
      return 0;
    }
}

/******************************************************************************
 * con_vc_server_gmode                                                        *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 * out: mode          ... CON_IN, CON_OUT, CON_INOUT                          *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Get mode of current virtual console                                        *
 *****************************************************************************/
l4_int32_t 
con_vc_server_gmode(sm_request_t *request, l4_uint8_t *mode, 
		    l4_uint32_t *sbuf1_size, l4_uint32_t *sbuf2_size, 
		    l4_uint32_t *sbuf3_size, sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  *mode       = vc->mode;
  *sbuf1_size = vc->sbuf1_size;
  *sbuf2_size = vc->sbuf2_size;
  *sbuf3_size = vc->sbuf3_size;
	
  return 0;
}

/******************************************************************************
 * con_vc_server_close                                                        *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Close current virtual console                                              *
 *****************************************************************************/
l4_int32_t 
con_vc_server_close(sm_request_t *request, 
		    sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);
  
  return vc_close(vc);
}

/******************************************************************************
 * con_vc_server_graph_smode                                                  *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      g_mode        ... graphics mode                                       *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Setup graphics mode of current virtual console                             *
 *****************************************************************************/
l4_int32_t 
con_vc_server_graph_smode(sm_request_t *request, 
			  l4_uint8_t g_mode, 
			  sm_exc_t *_ev)
{
  return -CON_ENOTIMPL;
}

/******************************************************************************
 * con_vc_server_graph_gmode                                                  *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 * out: g_mode        ... graphics mode                                       *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Get graphics mode of current virtual console                               *
 *****************************************************************************/
l4_int32_t 
con_vc_server_graph_gmode(sm_request_t *request, 
			  l4_uint8_t *g_mode,
			  l4_uint32_t *xres,
			  l4_uint32_t *yres,
			  l4_uint32_t *bits_per_pixel,
			  l4_uint32_t *bytes_per_pixel,
			  l4_uint32_t *bytes_per_line,
			  l4_uint32_t *flags,
			  l4_uint32_t *xtxt, l4_uint32_t *ytxt,
			  sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  *g_mode          = vc->gmode;
  *xres            = vc->user_xres;
  *yres            = vc->user_yres;
  *bits_per_pixel  = vc->bpp;
  *bytes_per_pixel = vc->bytes_per_pixel;
  *bytes_per_line  = vc->bytes_per_line;
  *flags           = vc->flags;
  *xtxt            = FONT_XRES;
  *ytxt            = FONT_YRES;
  return 0;
}


l4_int32_t
con_vc_server_graph_mapfb(sm_request_t *request, 
			  l4_snd_fpage_t *page,
			  l4_uint32_t *offset,
			  sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);
  l4_addr_t base = (l4_addr_t)vis_vmem & L4_SUPERPAGEMASK;
  l4_offs_t offs = (l4_addr_t)vis_vmem - base;

  /* XXX map more than 4 MB */

  page->fpage = l4_fpage(base, L4_LOG2_SUPERPAGESIZE,
			 L4_FPAGE_RW, L4_FPAGE_MAP);
  *offset = offs;
  vc->fb_mapped = 1;
  update_id = 1;

  return 0;
}


/******************************************************************************
 * con_vc_server_ev_sflt                                                      *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      filter        ... event filter                                        *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * description                                                                *
 *****************************************************************************/
l4_int32_t 
con_vc_server_ev_sflt(sm_request_t *request, 
		      l4_uint32_t filter, 
		      sm_exc_t *_ev)
{
  return -CON_ENOTIMPL;
}

/******************************************************************************
 * con_vc_server_ev_gflt                                                      *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 * out: filter        ... event filter                                        *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * description                                                                *
 *****************************************************************************/
l4_int32_t 
con_vc_server_ev_gflt(sm_request_t *request, 
		      l4_uint32_t *filter, 
		      sm_exc_t *_ev)
{
  return -CON_ENOTIMPL;
}

/******************************************************************************
 * con_vc_pslim - IDL server functions                                        *
 *****************************************************************************/

/******************************************************************************
 * con_vc_server_pslim_fill                                                   *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      rect          ... vfb area info                                       *
 *      color         ... fill color                                          *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Fill rectangular area of virtual framebuffer with color                    *
 *****************************************************************************/
l4_int32_t 
con_vc_server_pslim_fill(sm_request_t *request, 
			 const con_pslim_rect_t *rect, 
			 con_pslim_color_t color, 
			 sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  vc_fill(vc, 1, (pslim_rect_t*)rect, color);
  /* wait for any pending acceleration operation before return because the 
   * user has direct access to the framebuffer */
  if (vc->fb_mapped)
    vc->do_sync();
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}

/******************************************************************************
 * con_vc_server_pslim_copy                                                   *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      rect          ... vfb area info                                       *
 *      dx            ... destination x coordinate (may be negative)          *
 *      dy            ... dito for y coordinate                               *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Copy rectangular area of virtual framebuffer to (dx,dy)                    *
 *****************************************************************************/
l4_int32_t 
con_vc_server_pslim_copy(sm_request_t *request,
			 const con_pslim_rect_t *rect, 
			 l4_int16_t dx, l4_int16_t dy, 
			 sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;
  
  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_copy(vc, 1, (pslim_rect_t *) rect, dx, dy);
  /* wait for any pending acceleration operation before return because the 
   * user has direct access to the framebuffer */
  if (vc->fb_mapped)
    vc->do_sync();
  l4lock_unlock(&vc->fb_lock);

  return 0;
}

/******************************************************************************
 * con_vc_server_pslim_bmap                                                   *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      rect          ... vfb area info                                       *
 *      fg_color      ... foreground color                                    *
 *      bg_color      ... background color                                    *
 *      bmap          ... mask as bitmap                                      *
 *      bmap_type     ... type of bitmap: starting most or least significant  *
 *                        bit (START_MSB/_LSB)                                *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Set rectangular area of virtual framebuffer with foreground and background *
 * color mask in bitmap                                                       *
 *****************************************************************************/
l4_int32_t 
con_vc_server_pslim_bmap(sm_request_t *request,
			 const con_pslim_rect_t *rect,
			 con_pslim_color_t fg_color, 
			 con_pslim_color_t bg_color,
			 l4_strdope_t bmap, 
			 l4_uint8_t bmap_type, 
			 sm_exc_t *_ev)
{
  void *map = (void*)bmap.rcv_str;
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  convert_color(vc, &fg_color);
  convert_color(vc, &bg_color);

  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_bmap(vc, 1, (pslim_rect_t *) rect, 
	      fg_color, bg_color, 
	      map, bmap_type);
  l4lock_unlock(&vc->fb_lock);

  return 0;
}

/******************************************************************************
 * con_vc_server_pslim_set                                                    *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      rect          ... vfb area info                                       *
 *      pmap          ... pixmap                                              *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Set rectangular area of virtual framebuffer with color in pixelmap         *
 *****************************************************************************/
l4_int32_t 
con_vc_server_pslim_set(sm_request_t *request,
			const con_pslim_rect_t *rect, 
			l4_strdope_t pmap, 
			sm_exc_t *_ev)
{
  void *map = (void*)pmap.rcv_str;

  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_set(vc, 1, (pslim_rect_t *) rect, map);
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}

/******************************************************************************
 * con_vc_server_pslim_cscs                                                   *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      rect          ... vfb area info                                       *
 *      yuv           ... pixmap in YUV color space                           *
 *      yuv_type      ... type of pixmap's YUV encoding                       *
 *      scale         ... scale pixmap (defaults to 1) (NOT SUPPORTED YET)    *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Convert pixmap from YUV to RGB color space, scale and set rectangular area *
 * of virtual framebuffer                                                     *
 *****************************************************************************/
l4_int32_t 
con_vc_server_pslim_cscs(sm_request_t *request, 
			 const con_pslim_rect_t *rect,
			 l4_strdope_t y, l4_strdope_t u, l4_strdope_t v,
			 l4_uint8_t yuv_type, char scale, sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  if (scale != 1)
    return -CON_ENOTIMPL;

  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    {
      switch (yuv_type)
	{
	case pSLIM_CSCS_PLN_I420:
	  pslim_cscs(vc, 1, (pslim_rect_t *) rect,
		      (void*)y.rcv_str, (void*)v.rcv_str, (void*)u.rcv_str,
		      yuv_type, 1);
	  break;
	case pSLIM_CSCS_PLN_YV12:
	  pslim_cscs(vc, 1, (pslim_rect_t *) rect,
		      (void*)y.rcv_str, (void*)u.rcv_str, (void*)v.rcv_str,
		      yuv_type, 1);
	  break;
	}
    }
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}


/******************************************************************************
 * con_vc_server_puts                                                         *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      s             ... string                                              *
 *      x, y          ... coordinates for drawing (screen coordinates)        *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Set rectangular area of virtual framebuffer with color in pixelmap         *
 *****************************************************************************/
l4_int32_t 
con_vc_server_puts(sm_request_t *request,
		   l4_strdope_t s, 
		   l4_int16_t x, 
		   l4_int16_t y,
		   con_pslim_color_t fg_color, 
		   con_pslim_color_t bg_color, 
		   sm_exc_t *exc)
{
  l4_int32_t ret;
  struct l4con_vc *vc = (struct l4con_vc*) flick_server_get_local(request);
  
  l4lock_lock(&vc->fb_lock);
  ret = vc_puts(vc, 1, (l4_uint8_t*)s.rcv_str, s.snd_size,
		x, y, fg_color, bg_color);
  l4lock_unlock(&vc->fb_lock);

  return ret;
}


/******************************************************************************
 * con_vc_server_puts_attr                                                    *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      s             ... string                                              *
 *      x, y          ... coordinates for drawing (screen coordinates)        *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Set rectangular area of virtual framebuffer with color in pixelmap         *
 *****************************************************************************/
l4_int32_t 
con_vc_server_puts_attr(sm_request_t *request,
			l4_strdope_t s, 
			l4_int16_t x, 
			l4_int16_t y,
			sm_exc_t *exc)
{
  struct l4con_vc *vc;
  int i, j;
  l4_uint16_t* str = (l4_uint16_t*) s.rcv_str;
  
  vc = (struct l4con_vc *) flick_server_get_local(request);   
  
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    {
      for(i=0; i<s.snd_size; i+=2)
	{
	  const int c = *str++;
	  const pslim_color_t fgc = vc->color_tab[(c & 0x0F00) >> 8];
	  const pslim_color_t bgc = vc->color_tab[(c & 0xF000) >> 12];
	  
	  if ((c & 0xFF) == ' ')
	    {
	      /* optimization: collect spaces */
	      pslim_rect_t rect;
	      
	      for (j=1; (*str == c) && (i<s.snd_size-2); i+=2, j++, str++)
		;
	      rect = (pslim_rect_t) { x, y, j*FONT_XRES, FONT_YRES };
	      pslim_fill(vc, 1, &rect, bgc);
	      x += j*FONT_XRES;
	    }
	  else
	    {
	      pslim_rect_t rect = { x, y, FONT_XRES, FONT_YRES };
	      
	      pslim_bmap(vc, 1,
			 &rect, fgc, bgc,
			 (void*)&_binary_font_psf_start[rect.h * (c & 0xFF)+4],
			 pSLIM_BMAP_START_MSB);
	      x += FONT_XRES;
	    }
	}
    }
  /* wait for any pending acceleration operation before return because the 
   * user can directly access the framebuffer */
  if (vc->fb_mapped)
    vc->do_sync();
  l4lock_unlock(&vc->fb_lock);
  return 0;
}


l4_int32_t
con_vc_server_direct_update(sm_request_t *request, 
			    const con_pslim_rect_t *rect, 
			    sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);
  
  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  if (vc->vfb == 0)
    {
      printf("no vfb set\n");
      return -CON_EPERM;
    }
  
  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_set(vc, 1, (pslim_rect_t *) rect, 0 /* use mapped vfb */);
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}


l4_int32_t
con_vc_server_direct_setfb(sm_request_t *request,
			   const con_dataspace_t *data_ds,
			   sm_exc_t *_ev)
{
  int error;
  l4_size_t size;

  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  if (vc->vfb_used)
    {
      INFO("Virtual framebuffer used -- direct_setfb nonsense\n");
      return -EINVAL;
    }

  if (vc->fb_mapped)
    {
      INFO("Physical framebuffer mapped -- direct_setfb nonsense\n");
      return -EINVAL;
    }
  
  if ((error = l4dm_mem_size((l4dm_dataspace_t*)data_ds, &size)))
    {
      INFO("Error %d requesting size of data_ds\n", error);
      return -EINVAL;
    }
  
  if ((error = l4rm_attach((l4dm_dataspace_t*)data_ds, size, 0, L4DM_RO,
			   &vc->vfb)))
    {
      INFO("Error %d attaching data_ds\n", error);
      return -EINVAL;
    }

  return 0;
}

/******************************************************************************
 * con_vc_server_stream_cscs                                                  *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Convert pixmap from YUV to RGB color space, scale and set rectangular area *
 * of virtual framebuffer                                                     *
 *****************************************************************************/
l4_int32_t 
con_vc_server_stream_cscs(sm_request_t *request,
			  const con_pslim_rect_t *rect_src,
			  const con_pslim_rect_t *rect_dst,
			  l4_uint8_t yuv_type,
			  l4_snd_fpage_t *buffer,
			  l4_uint32_t *offs_y, l4_uint32_t *offs_u, 
			  l4_uint32_t *offs_v,
			  sm_exc_t *_ev)
{
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);
  vidix_playback_t config;

  if (!hw_accel.caps & ACCEL_FAST_CSCS)
    {
      printf("No hardware acceleration for cscs available\n");
      return -CON_ENOTIMPL;
    }

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  switch (yuv_type)
    {
    case pSLIM_CSCS_PLN_I420:
      if (!hw_accel.caps & ACCEL_FAST_CSCS_YV12)
	return -CON_ENOTIMPL;
      config.fourcc = IMGFMT_I420;
      break;
    case pSLIM_CSCS_PLN_YV12:
      if (!hw_accel.caps & ACCEL_FAST_CSCS_YV12)
	return -CON_ENOTIMPL;
      config.fourcc = IMGFMT_YV12;
      break;
    case pSLIM_CSCS_PCK_YUY2:
      if (!hw_accel.caps & ACCEL_FAST_CSCS_YUY2)
	return -CON_ENOTIMPL;
      config.fourcc = IMGFMT_YUY2;
      break;
    default:
      return -CON_ENOTIMPL;
    }

  config.src    = (vidix_rect_t){ rect_src->x, rect_src->y,
				  rect_src->w, rect_src->h, { 0, 0, 0 } };
  config.dest   = (vidix_rect_t){ rect_dst->x, rect_src->y,
				  rect_dst->w, rect_dst->h, { 0, 0, 0 } };

  if (hw_accel.caps & ACCEL_COLOR_KEY)
    {
      /* use color key */
      static vidix_grkey_t gr_key;

      gr_key.key_op = KEYS_PUT;
      gr_key.ckey.op = CKEY_TRUE;
      gr_key.ckey.red   = 0xFF;
      gr_key.ckey.green = 0x00;
      gr_key.ckey.blue  = 0xFF;
      hw_accel.cscs_grkey(&gr_key);
    }

  config.num_frames = 1;
  hw_accel.cscs_init(&config);

  *offs_y = config.offsets[0] + config.offset.y;
  *offs_u = config.offsets[0] + config.offset.u;
  *offs_v = config.offsets[0] + config.offset.v;

  /* set offscreen area to "black" */
  switch (yuv_type)
    {
    case pSLIM_CSCS_PLN_I420:
    case pSLIM_CSCS_PLN_YV12:
	{
	  unsigned size = ((rect_src->w+31)&~31)*rect_src->h;
	  memset((void*)config.dga_addr+*offs_y, 0, size);
	  memset((void*)config.dga_addr+*offs_u, 128, size/4);
	  memset((void*)config.dga_addr+*offs_v, 128, size/4);
	}
      break;
    case pSLIM_CSCS_PCK_YUY2:
	{
	  unsigned stride = 2*((rect_src->w+15)&~15);
	  unsigned h_size = rect_src->h, w_size = rect_src->w>>1;
	  unsigned char *dest = (unsigned char*)config.dga_addr+*offs_y;
	  int i;
	  for (i=0; i<h_size; i++)
	    {
	      int j;
	      for (j=0; j<w_size; j++)
		((unsigned int*)dest)[j] = 0x80008000;
	      dest += stride;
	    }
	}
      break;
    }
  
  hw_accel.cscs_start();

  if (hw_accel.caps & ACCEL_EQUALIZER)
    {
      /* use equalizer */
      cscs_eq.cap = VEQ_CAP_BRIGHTNESS | VEQ_CAP_CONTRAST;
      cscs_eq.brightness = 300;
      cscs_eq.contrast   = 300;
      hw_accel.cscs_eq(&cscs_eq);
    }

  if (hw_accel.caps & ACCEL_COLOR_KEY)
    {
      /* make video visible by filling area using colorkey-color */
      pslim_rect_t rect = { config.dest.x, config.dest.y, 
			    config.dest.w, config.dest.h };
      pslim_color_t pink = { 0x00FF00FF };

      l4lock_lock(&vc->fb_lock);

      convert_color(vc, &pink);
      pslim_fill(vc, 0, &rect, pink);
      vc_puts_scale(vc, 0, "DROPS", 5, 20, 20, 0, 0x00ff00ff, 1, 2);

      l4lock_unlock(&vc->fb_lock);
    }

  printf("Opening cscs stream %dx%d => %dx%d\n",
      config.src.w, config.src.h, config.dest.w, config.dest.h);
  buffer->fpage = l4_fpage((l4_uint32_t)config.dga_addr, bsr(config.frame_size),
			   L4_FPAGE_RW, L4_FPAGE_MAP);

  cscs_stream_active = 1;

  return 0;
}


void
vc_brightness_contrast(int diff_brightness, int diff_contrast)
{
  if (hw_accel.caps & ACCEL_EQUALIZER)
    {
      cscs_eq.cap = VEQ_CAP_BRIGHTNESS | VEQ_CAP_CONTRAST;
      cscs_eq.brightness += diff_brightness;
      if (cscs_eq.brightness > 1000)
	cscs_eq.brightness = 1000;
      if (cscs_eq.brightness < -1000)
	cscs_eq.brightness = -1000;
      cscs_eq.contrast += diff_contrast;
      if (cscs_eq.contrast > 1000)
	cscs_eq.contrast = 1000;
      if (cscs_eq.contrast < -1000)
	cscs_eq.contrast = -1000;
      hw_accel.cscs_eq(&cscs_eq);
    }
}



/******************************************************************************
 * vc_loop                                                                    *
 *                                                                            *
 * con_vc - IDL server loop                                                   *
 *****************************************************************************/
void 
vc_loop(struct l4con_vc *this_vc)
{
  int ret, str_cnt;
  l4_msgdope_t result;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;

  flick_init_request(&request, &ipc_buf);

  /* Allow maximal three indirect string (refstring) to be sent to us. */
  str_cnt = 1;
  flick_server_set_rcvstring(&request, 0, 
			     this_vc->sbuf1_size, this_vc->sbuf1);

  if ((this_vc->sbuf2_size != 0) && (this_vc->sbuf3_size != 0))
    {
      flick_server_set_rcvstring(&request, 1, 
				  this_vc->sbuf2_size, this_vc->sbuf2);
      flick_server_set_rcvstring(&request, 2, 
				  this_vc->sbuf3_size, this_vc->sbuf3);
      str_cnt += 2;
    }
  
  flick_set_number_rcvstring(&request, str_cnt);
  
  /* pass this_vc reference as implicit arg */
  flick_server_set_local(&request, (void*)this_vc);

  /* set snd timeout 0 if client disappears before sending the answer */
  flick_server_set_timeout(&request, L4_IPC_TIMEOUT(0,1,0,0,0,0));

  /* tell creator that we are running */
  l4thread_started(NULL);

  INFO("vc[%d] running as %x.%02x\n",
	this_vc->vc_number, 
   	l4thread_l4_id(l4thread_myself()).id.task,
	l4thread_l4_id(l4thread_myself()).id.lthread);

  /* IDL server loop */
  while (1)
    {
      result = flick_server_wait(&request);
      while (!L4_IPC_IS_ERROR(result))
	{
	  /* dispatch request */
	  ret = con_vc_server(&request);

	  if (this_vc->mode == CON_CLOSING) 
	    {
	      /* send answer */
	      result = flick_server_send(&request);

	      /* mark vc as free */
	      this_vc->mode = CON_CLOSED;

	      /* stop thread ... there should be no problem 
	       * if main_thread races here, since everything 
	       * is done for now. */
	      l4thread_exit();
	    }

	  switch(ret) 
	    {
	    case DISPATCH_ACK_SEND:
	      result = flick_server_reply_and_wait(&request);
	      break;
	      
    	    default:
	      INFO("Flick dispatch error (%d)!\n", ret);
	      result = flick_server_wait(&request);
	      break;
	    }
	} /* !L4_IPC_IS_ERROR(result) */

      if (L4_IPC_ERROR(result) == L4_IPC_ENOT_EXISTENT)
	{
	  /* application was killed ==> close vc */
	  want_vc = this_vc->vc_number | 0x1000;

	  INFO("Partner thread killed, closing console %d\n", 
	      this_vc->vc_number & ~0x1000);
	  
	  continue;
	}
      else if (L4_IPC_ERROR(result) == L4_IPC_SETIMEOUT)
	{
	  /* client did not wait for anwser, maybe it was lthread_ex_regs'd */
	  INFO("Timeout replying to %x.%x\n",
	       request.client_tid.id.task, request.client_tid.id.lthread);
	  continue;
	}

      PANIC("Flick IPC error %#x client=%x.%x",
	       L4_IPC_ERROR(result),
	       request.client_tid.id.task, request.client_tid.id.lthread);
    }
}

