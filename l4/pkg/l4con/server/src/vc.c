/* $Id$ */
/**
 * \file	con/server/src/vc.c
 * \brief	virtual console stuff
 *		ATTENTION: it's multi threaded
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

/* L4 includes */
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#if defined(ARCH_x86) || defined(ARCH_amd64)
#include <l4/util/rdtsc.h>
#include <l4/util/idt.h>
#include <l4/generic_io/libio.h>
#endif
#include "l4/l4con/l4con-server.h"

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "config.h"
#include "gmode.h"
#include "main.h"
#include "pslim.h"
#include "vc.h"
#include "ev.h"
#include "con_hw/fourcc.h"
#include "con_hw/vidix.h"
#include "con_yuv2rgb/yuv2rgb.h"

extern const char _binary_font_psf_start[];
l4_umword_t status_area = 0;

static vidix_video_eq_t cscs_eq;

static int vc_open_in(struct l4con_vc*);
static int vc_open_out(struct l4con_vc*);

/* dummy function for sync, does nothing */
static void nothing_sync(void)
{}

pslim_copy_fn fg_do_copy = sw_copy;
pslim_copy_fn bg_do_copy = sw_copy;
pslim_fill_fn fg_do_fill = sw_fill;
pslim_fill_fn bg_do_fill = sw_fill;
pslim_sync_fn fg_do_sync = nothing_sync;
pslim_sync_fn bg_do_sync = nothing_sync;
pslim_drty_fn fg_do_drty;

con_accel_t hw_accel =
{ copy:sw_copy, fill:sw_fill, sync:nothing_sync };

/** Color tables for puts_attr. */
static const l4con_pslim_color_t color_tab15[16] =
{ 
  0x0000, 0x0015, 0x02a0, 0x02b5, 0x5400, 0x5415, 0x5540, 0x56b5,
  0x294a, 0x295f, 0x2bea, 0x2bff, 0x7d4a, 0x7d5f, 0x7fea, 0x7fff
};
static const l4con_pslim_color_t color_tab16[16] =
{ 
  0x0000, 0x0015, 0x0540, 0x0555, 0xa800, 0xa815, 0xaaa0, 0xad55,
  0x52aa, 0x52bf, 0x57ea, 0x57ff, 0xfaaa, 0xfabf, 0xffea, 0xffff
};
static const l4con_pslim_color_t color_tab32[16] =
{
  0x00000000, 0x000000aa, 0x0000aa00, 0x0000aaaa,
  0x00aa0000, 0x00aa00aa, 0x00aa5500, 0x00aaaaaa,
  0x00555555, 0x005555ff, 0x0055ff55, 0x0055ffff,
  0x00ff5555, 0x00ff55ff, 0x00ffff55, 0x00ffffff
};

/** Convert l4con_pslim_color_t into ``drawable'' color. */
static inline void
convert_color(struct l4con_vc *vc, l4con_pslim_color_t *color)
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
	  *color = (((*color >> (16 + 8 -   VESA_RED_SIZE)) & ((1 <<   VESA_RED_SIZE)-1)) << VESA_RED_OFFS)
	         | (((*color >> ( 8 + 8 - VESA_GREEN_SIZE)) & ((1 << VESA_GREEN_SIZE)-1)) << VESA_GREEN_OFFS)
	         | (((*color >> ( 0 + 8 -  VESA_BLUE_SIZE)) & ((1 <<  VESA_BLUE_SIZE)-1)) << VESA_BLUE_OFFS);
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

static int
vc_l4io_init(void)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  l4io_info_t *io_info_addr = (l4io_info_t*)0;

  if (l4io_init(&io_info_addr, L4IO_DRV_INVALID))
    {
      Panic("Couldn't connect to L4 IO server!");
      return 1;
    }
  if (!io_info_addr->omega0)
    Panic("l4io has no omega0 mode enabled!");
#endif

  return 0;
}

/** Init graphics console driver */
static void 
vc_init_gr(void)
{
  init_gmode();

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

/** Init font */
static int
vc_font_init(void)
{
  /* all consoles can use puts interface */
  /* check magic number of .psf */
  if (_binary_font_psf_start[0] != 0x36 || _binary_font_psf_start[1] != 0x04)
    Panic("psf magic number failed");

  FONT_XRES = 8;
  FONT_YRES = _binary_font_psf_start[3];

  /* check file mode */
  switch (_binary_font_psf_start[2]) 
    {
    case 0:
    case 2:
      FONT_CHRS = 256;
      break;
    case 1:
    case 3:
      FONT_CHRS = 512; 
      break;
    default:
      Panic("bad psf font file magic %02x!", _binary_font_psf_start[2]);
    }

  LOG_printf("Character size is %dx%d, font has %d characters\n", 
	     FONT_XRES, FONT_YRES, FONT_CHRS);

  status_area = FONT_YRES + 4;

  return 0;
}

void 
vc_init()
{
  int i;

  if (!use_s0)
    vc_l4io_init();
  vc_font_init();
  vc_init_gr();

  for (i = 0; i < MAX_NR_L4CONS; i++) 
    {
      /* malloc */
      vc[i] = malloc(sizeof(struct l4con_vc));
      if (!vc[i])
	{
	  LOG_printf("Out of memory allocating l4con_vc");
	  exit(-1);
	}

      /* init values */
      vc[i]->prev      = 0;
      vc[i]->next      = 0;
      vc[i]->vc_number = i;
      vc[i]->mode      = CON_CLOSED;
      vc[i]->vfb       = 0;
      vc[i]->fb_lock   = L4LOCK_UNLOCKED;
    }

  /* the master console is special. It is shown if no other vc's are open */
  vc[0]->next            =
  vc[0]->prev            = vc[0];
  vc[0]->mode            = CON_MASTER | CON_OUT;
  vc[0]->vfb             = 0;
  vc[0]->vfb_in_server   = 0;
  vc[0]->fb              = gr_vmem;
  vc[0]->fb_mapped       = 0;
  vc[0]->vc_partner_l4id = L4_INVALID_ID;
  vc[0]->vc_l4id         = L4_INVALID_ID;
  vc[0]->ev_partner_l4id = L4_NIL_ID;
  vc[0]->gmode           = VESA_RES;
  vc[0]->xres            = VESA_XRES;
  vc[0]->yres            = VESA_YRES;
  vc[0]->pan_xofs        = pan_offs_x;
  vc[0]->pan_yofs        = pan_offs_y;
  vc[0]->client_xofs     = 0;
  vc[0]->client_yofs     = 0;
  vc[0]->client_xres     = VESA_XRES;
  vc[0]->client_yres     = VESA_YRES_CLIENT;
  vc[0]->logo_x          = 100000;
  vc[0]->logo_y          = 100000;
  vc[0]->bpp             = VESA_BITS;
  vc[0]->bytes_per_pixel = (VESA_BITS+7)/8;
  vc[0]->bytes_per_line  = VESA_BPL;
  vc[0]->flags           = accel_caps;
  vc[0]->do_sync         = fg_do_sync;
  vc[0]->do_drty         = fg_do_drty;
}

/**
 * Open new virtual console (already set to CON_OPENING).
 *
 * \param vc         current information structure
 * \param mode       CON_OUT, CON_INOUT
 * \param ev_handler event handler thread
 * return 0          success
 */
static int
vc_open(struct l4con_vc *vc, l4_uint8_t mode, l4_threadid_t ev_handler)
{
  int error;
  l4_threadid_t e = L4_NIL_ID;

  switch (mode)
    {
    case CON_INOUT:
      e = ev_handler;

      if ((error = vc_open_in(vc))) 
	return error;

      // fall through

    case CON_OUT:
      vc->ev_partner_l4id = e;

      if ((error = vc_open_out(vc)))
	return error;

      vc->mode = mode;

      /* switch to new opened vc */
      request_vc(vc->vc_number);
      break;
    }

  return 0;
}

/**
 * Do CON_IN part of open.
 * \param vc      current information structure
 * \return   0    success
 *          -1    failed */
int
vc_open_in(struct l4con_vc *vc)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* is there an event handler thread and is he `alive' */
  if (l4_ipc_receive(vc->ev_partner_l4id, L4_IPC_SHORT_MSG, &dummy, &dummy,
		     L4_IPC_RECV_TIMEOUT_0, &result) == L4_IPC_ENOT_EXISTENT)
    return -CON_ETHREAD;

  return 0;
}

/**
 * Do CON_OUT part of open.
 * \param vc      current information structure
 * \return  0     success
 *         -1     failed */
int 
vc_open_out(struct l4con_vc *vc)
{
  int i;

  /* set default video mode */
  vc->gmode           = VESA_RES;
  vc->client_xofs     = 0;
  vc->client_yofs     = 0;
  vc->client_xres     = VESA_XRES;
  vc->client_yres     = VESA_YRES_CLIENT;
  vc->xres            = VESA_XRES;
  vc->yres            = VESA_YRES;
  vc->pan_xofs        = pan_offs_x;
  vc->pan_yofs        = pan_offs_y;
  vc->bpp             = VESA_BITS;
  vc->logo_x          = 100000;
  vc->logo_y          = 100000;
  vc->bytes_per_pixel = (vc->bpp+7)/8;
  vc->bytes_per_line  = VESA_BPL;
  vc->vfb_size        = ((vc->client_yres * vc->bytes_per_line) + 3) & ~3;
  vc->flags           = accel_caps;
  vc->do_copy         = bg_do_copy;
  vc->do_fill         = bg_do_fill;
  vc->do_sync         = bg_do_sync;
  vc->do_drty         = 0;

  for (i=0; i<CONFIG_MAX_CLIENTS; i++)
    vc->clients[i] = L4_INVALID_ID;

  switch (vc->gmode & GRAPH_BPPMASK)
    {
    case GRAPH_BPP_32:
    case GRAPH_BPP_24: vc->color_tab = color_tab32; break;
    case GRAPH_BPP_15: vc->color_tab = color_tab15; break;
    case GRAPH_BPP_16:
    default:           vc->color_tab = color_tab16; break;
    }

  if (vc->vfb_in_server)
    {
      int error;
      char ds_name[32];
      l4dm_dataspace_t ds;

      sprintf(ds_name, "vfb for "l4util_idfmt, 
	      l4util_idstr(vc->vc_partner_l4id));
      if ((error = l4dm_mem_open(L4DM_DEFAULT_DSM, vc->vfb_size, 
				 0, 0, ds_name, &ds)))
	{
	  LOG("Error %d requesting %d bytes for vc", error, vc->vfb_size);
	  Panic("open_vc_out");
	  return -CON_ENOMEM;
	}
      if ((error = l4rm_attach(&ds, vc->vfb_size, 0,
                               L4DM_RW | L4RM_SUPERPAGE_ALIGNED,
			       (void**)&vc->vfb)))
	{
	  LOG("Error %d attaching vc dataspace", error);
	  Panic("open_vc_out");
	}

      memset(vc->vfb, 0, vc->vfb_size);
      vc->fb       = vc->vfb;
      vc->pan_xofs = 0;
      vc->pan_yofs = 0;
    }

  LOG_printf("vc[%d] %ldx%ld@%d, bpl:%ld, gmode:0x%x, evprt:"l4util_idfmt
             " save:%d\n", vc->vc_number, vc->xres, vc->yres, vc->bpp,
             vc->bytes_per_line, vc->gmode, l4util_idstr(vc->ev_partner_l4id),
             vc->save_restore);
  return 0;
}

/**
 * Close virtual console.
 * \param this_vc information structure
 * \return  0     success
 *         -1     failed */
int 
vc_close(struct l4con_vc *this_vc)
{
  /* XXX notify event thread of partner */

  /* make sure that the main thread cannot access our vfb */
  l4lock_lock(&this_vc->fb_lock);
  /* temporary mode: no output allowed, but occupied */
  this_vc->mode = CON_CLOSING;
  if (this_vc->vfb_in_server && this_vc->vfb)
    {
      l4dm_mem_release(this_vc->vfb);
      this_vc->vfb = 0;
    }
  this_vc->vfb_in_server = 0;
  this_vc->fb = 0;
  l4lock_unlock(&this_vc->fb_lock);

  if (this_vc->sbuf1)
    {
      l4dm_mem_release(this_vc->sbuf1);
      this_vc->sbuf1 = 0;
    }
  if (this_vc->sbuf2)
    {
      l4dm_mem_release(this_vc->sbuf2);
      this_vc->sbuf2 = 0;
    }
  if (this_vc->sbuf3)
    {
      l4dm_mem_release(this_vc->sbuf3);
      this_vc->sbuf3 = 0;
    }

  this_vc->ev_partner_l4id = L4_NIL_ID;

  if (this_vc->vc_number == fg_vc)
    request_vc(-1);
  else
    update_id = 1;

  return 0;
}

/** Render string to screen.
 * @pre have vc->fb_lock */
static int
vc_puts(struct l4con_vc *vc, int from_user,
	const char *str, int len, l4_int16_t x, l4_int16_t y,
	l4con_pslim_color_t fg_color, l4con_pslim_color_t bg_color)
{
  int i, j;

  convert_color(vc, &fg_color);
  convert_color(vc, &bg_color);

  if(vc->fb == 0)
    return 0;

  for (i=0; i<len; i++, str++)
    {	
      /* optimization: collect spaces */
      for (j=0; (i<len) && (*str == ' '); i++, j++, str++)
	;	  

      if (j>0)
	{
	  l4con_pslim_rect_t rect = { x, y, j*FONT_XRES, FONT_YRES };

	  pslim_fill(vc, from_user, &rect, bg_color);
	  x += j*FONT_XRES;
	  i--; str--;
	}
      else
	{
	  l4con_pslim_rect_t rect = { x, y, FONT_XRES, FONT_YRES };

	  pslim_bmap(vc, from_user, &rect, fg_color, bg_color, 
		     (void*) &_binary_font_psf_start[rect.h * (*str)+4],
		     pSLIM_BMAP_START_MSB);
	  x += FONT_XRES;
	}
    }

  return 0;
}

/** fill rectangle of screen.
 * @pre have vc->fb_lock */
static int
vc_fill(struct l4con_vc *vc, int from_user,
	l4con_pslim_rect_t *rect, l4con_pslim_color_t color)
{
  if (!(vc->mode & CON_OUT))
    return -CON_EPERM;

  convert_color(vc, &color);

  if (vc->fb != 0)
    pslim_fill(vc, from_user, rect, color);

  return 0;
}

/** Put characters with scale >= 1.
 * @pre have vc->fb_lock */
static int
vc_puts_scale(struct l4con_vc *vc, int from_user,
	      const char *str, int len, l4_int16_t x, l4_int16_t y,
	      l4con_pslim_color_t fg_color, l4con_pslim_color_t bg_color,
	      int scale_x, int scale_y)
{
  int pix_x, pix_y;
  l4con_pslim_rect_t rect = { x, y, FONT_XRES*scale_x, FONT_YRES*scale_y };

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
	  l4con_pslim_rect_t lrect = { rect.x, rect.y, pix_x, pix_y };
	  const char *bmap = &_binary_font_psf_start[FONT_YRES*(*str) + 4];
	  int j;

	  for (j=0; j<FONT_YRES; j++)
	    {
	      unsigned char mask = 0x80;
	      int k;

	      for (k=0; k<FONT_XRES; k++)
		{
		  l4con_pslim_color_t color = (*bmap & mask) ? fg_color
							     : bg_color;
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

/** Show id of current console at bottom of screen.
 * @pre have vc->fb_lock */
void
vc_show_id(struct l4con_vc *this_vc)
{
  char id[64];
  int i, x, cnt_vc;
  const l4con_pslim_color_t fgc = 0x009999FF;
  const l4con_pslim_color_t bgc = 0x00666666;
  l4con_pslim_rect_t rect = { 0, this_vc->client_yres,
			      this_vc->xres, status_area };

  cnt_vc = MAX_NR_L4CONS > 10 ? 9 : MAX_NR_L4CONS-1;

  vc_fill(this_vc, 0, &rect, bgc);

  if (this_vc->vc_number != 0)
    sprintf(id, "TUDOS console: partner "l4util_idfmt"   ",
	l4util_idstr(this_vc->vc_partner_l4id));
  else
    strcpy(id, "TUDOS console: (all closed)");

  vc_puts(this_vc, 0, id, strlen(id), 2, this_vc->client_yres+2, fgc, bgc);

  sprintf(id, "%ldx%ld@%d%s%s", 
	  this_vc->xres, this_vc->yres, this_vc->bpp,
	  panned ?             " [PAN]"   : "",
	  this_vc->fb_mapped ? " [FBmap]" : "");
  vc_puts(this_vc, 0, id, strlen(id),
	  ((this_vc->xres - strlen(id)*FONT_XRES) / 2), this_vc->client_yres+2,
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

void
vc_show_dmphys_poolsize(struct l4con_vc *this_vc)
{
  const l4con_pslim_color_t fgc = 0x009999FF;
  const l4con_pslim_color_t bgc = 0x00666666;
  char  str[32];
  l4_size_t size, free;

  l4dm_memphys_poolsize(L4DM_MEMPHYS_DEFAULT, &size, &free);
  l4con_pslim_rect_t rect = 
    { this_vc->xres - 180, this_vc->client_yres, 80, status_area };
  vc_fill(this_vc, 0, &rect, bgc);
  sprintf(str, "%3zd/%zdMB", free/(1<<20), size/(1<<20));
  vc_puts(this_vc, 0, str, strlen(str), rect.x, rect.y+2, fgc, bgc);
}

#if defined(ARCH_x86) || defined(ARCH_amd64)
static void
show_counters(struct l4con_vc *this_vc)
{
#if 0
  l4_tracebuffer_status_t *tb = fiasco_tbuf_get_status();

  l4con_pslim_rect_t rect = { this_vc->xres-380, this_vc->client_yres,
			      14*8, status_area };

  char  str[32];
  const l4con_pslim_color_t fgc = 0x009999FF;
  const l4con_pslim_color_t bgc = 0x00666666;

  vc_fill(this_vc, 0, &rect, bgc);
  sprintf(str, "%6ld %6ld", tb->cnt_context_switch, tb->cnt_addr_space_switch);
  vc_puts(this_vc, 0, str, strlen(str), rect.x, rect.y+2, fgc, bgc);
#endif
}
#endif

static void
vc_show_history(struct l4con_vc *this_vc,
		l4_uint8_t *history_val, l4_size_t history_size,
                l4con_pslim_color_t fgc, l4con_pslim_color_t bgc,
		l4con_pslim_rect_t *rect)
{
  /* show load history */
  int i, j;
  l4_uint8_t b;
  l4_uint8_t history_map[history_size/8*status_area];

  convert_color(this_vc, &fgc);
  convert_color(this_vc, &bgc);

  for (i=0, b=0x80; i<history_size; i++, b = (b>>1)|(b<<7))
    {
      for (j=0; j<status_area; j++)
	if (history_val[i] >= status_area-j)
	  history_map[i/8 + history_size/8*j] |= b;
	else
	  history_map[i/8 + history_size/8*j] &= ~b;
    }
  pslim_bmap(this_vc, 0, rect, fgc, bgc, history_map, pSLIM_BMAP_START_MSB);
}

void
vc_show_cpu_load(struct l4con_vc *this_vc)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  static l4_uint32_t tsc, pmc;
  static l4_uint8_t  history_val[6*8]; // 6 characters == "xxx.x%"!
  l4_uint32_t new_tsc = l4_rdtsc_32(), new_pmc = l4_rdpmc_32(0);
  l4con_pslim_rect_t rect = { this_vc->xres-260, this_vc->client_yres,
			      sizeof(history_val), status_area };

  memmove(history_val, history_val+1, sizeof(history_val)-1);

  show_counters(this_vc);

  if (!tsc || !pmc)
    history_val[sizeof(history_val)-1] = 0;

  else
    {
      history_val[sizeof(history_val)-1] = (new_pmc-pmc) / 
					   ((new_tsc-tsc)/status_area);

      if (cpu_load_history == 0)
	{
	  /* show value (0..100%) */
	  char  str[16];
	  const l4con_pslim_color_t fgc = 0x009999FF;
	  const l4con_pslim_color_t bgc = 0x00666666;

	  l4_uint32_t load = (new_pmc-pmc) / ((new_tsc-tsc)/1000);
	  vc_fill(this_vc, 0, &rect, bgc);
	  if (load > 1000)
	    strcpy(str, "---.-%");
	  else
	    sprintf(str, "%3d.%d%%", load/10, load % 10);
	  vc_puts(this_vc, 0, str, strlen(str), rect.x, rect.y+2, fgc, bgc);
	}
      else
	vc_show_history(this_vc, history_val, sizeof(history_val),
			0x00CC5555, 0x00222222, &rect);
    }

  tsc = new_tsc;
  pmc = new_pmc;
#endif
}

void
vc_show_drops_cscs_logo(void)
{
  if (vc[fg_vc]->logo_x != 100000)
    {
      static l4con_pslim_color_t color = 0x00050505;
      static l4con_pslim_color_t adder = 0x00050301;

      color += adder;
      if (color > 0x00f0f0f0)
	adder = -adder;
      else if (color < 0x00050505)
	adder = -adder;

      vc_puts_scale(vc[fg_vc], 0, "DROPS", 5,
		    vc[fg_vc]->logo_x, vc[fg_vc]->logo_y,
		    color, 0x00ff00ff, 1, 2);
    }
}

/** Clear vc.
 * @pre have vc->fb_lock */
void
vc_clear(struct l4con_vc *vc)
{
  const l4con_pslim_color_t fgc = 0x00223344;
  const l4con_pslim_color_t bgc = 0x00000000;
  l4con_pslim_rect_t rect = { 0, 0, vc->client_xres, vc->client_yres };
  
  vc_fill(vc, 0, &rect, bgc);

  /* special case is console 0 */
  if (vc->vc_number == 0)
    {
      /* master console, show DROPS label */
      int x, y, scale_x_1, scale_y_1, scale_x_2, scale_y_2;

      scale_x_1 = (vc->client_xres*4/ 5) / (5*FONT_XRES);
      scale_y_1 = (vc->client_yres*6/10) / (1*FONT_YRES);
      x = vc->client_xofs + (vc->client_xres-5*FONT_XRES*scale_x_1)/2;
      y = vc->client_yofs + (vc->client_yres-1*FONT_YRES*scale_y_1)*3/7;
      vc_puts_scale(vc, 0, 
			"TUDOS", 5,
		        x, y, fgc, bgc, scale_x_1, scale_y_1);
      scale_x_2 = scale_x_1*10/90;
      scale_y_2 = scale_y_1*10/90;
      x = vc->client_xofs + (vc->client_xres-36*FONT_XRES*scale_x_2)/2;
      y += 1*FONT_YRES*scale_y_1*12/14;
      vc_puts_scale(vc, 0, 
			"The Dresden Operating System Project", 36,
    			x, y, fgc, bgc, scale_x_2, scale_y_2);
    }
}

/**
 * Setup mode of current virtual console: input, output or in/out */
long
con_vc_smode_component (CORBA_Object _dice_corba_obj,
                        unsigned char mode,
                        const l4_threadid_t *ev_handler,
                        CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc*)(_dice_corba_env->user_data);
  
  if (vc->mode == CON_OPENING)
    {
      /* inital state */
      vc->ev_partner_l4id = *ev_handler;
      return vc_open(vc, mode & CON_INOUT, *ev_handler);
    }
  else 
    { 
      /* set new event handler */
      vc->ev_partner_l4id = mode & CON_IN ? *ev_handler : L4_NIL_ID;
      return 0;
    }
}

/**
 * Get mode of current virtual console */
long
con_vc_gmode_component (CORBA_Object _dice_corba_obj,
                        unsigned char *mode,
                        unsigned long *sbuf_1size,
                        unsigned long *sbuf_2size,
                        unsigned long *sbuf_3size,
                        CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);

  *mode       = vc->mode;
  *sbuf_1size = vc->sbuf1_size;
  *sbuf_2size = vc->sbuf2_size;
  *sbuf_3size = vc->sbuf3_size;
	
  return 0;
}

long
con_vc_share_component (CORBA_Object _dice_corba_obj,
                        const l4_threadid_t *client,
                        CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  int i;

  for (i=0; i<CONFIG_MAX_CLIENTS; i++)
    if (l4_thread_equal(*client, vc->clients[i]))
      return 0;
    else if (l4_is_invalid_id(vc->clients[i]))
      {
        vc->clients[i] = *client;
        return 0;
      }

  return -L4_ENOSPC;
}

long
con_vc_revoke_component (CORBA_Object _dice_corba_obj,
                         const l4_threadid_t *client,
                         CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  int i;

  for (i=0; i<CONFIG_MAX_CLIENTS; i++)
    if (l4_thread_equal(*client, vc->clients[i]))
      {
        vc->clients[i] = L4_INVALID_ID;
        return 0;
      }

  return -L4_ENOTFOUND;
}

/**
 * Close current virtual console */
long 
con_vc_close_component(CORBA_Object _dice_corba_obj,
		       l4_int16_t *_dice_reply,
		       CORBA_Server_Environment *_dice_corba_env)
{
  long ret;
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  
  ret = vc_close(vc);
  if (vc->mode == CON_CLOSING)
    {
      /* mark vc as free */
      vc->mode = CON_CLOSED;
      
      /* send answer */
      con_vc_close_reply(_dice_corba_obj, ret, _dice_corba_env);
      
      /* stop thread ... there should be no problem 
       * if main_thread races here, since everything 
       * is done for now. */
      l4thread_exit();
    }

  /* If we didn't close the console, return the return value 
   * and proceed. */
  return ret;
}

/**
 * Setup graphics mode of current virtual console. */
long 
con_vc_graph_smode_component(CORBA_Object _dice_corba_obj,
			     l4_uint8_t g_mode,
			     CORBA_Server_Environment *_dice_corba_env)
{
  return -CON_ENOTIMPL;
}

/**
 * Get graphics mode of current virtual console. */
long 
con_vc_graph_gmode_component(CORBA_Object _dice_corba_obj,
			     l4_uint8_t *g_mode,
			     l4_uint32_t *xres,
			     l4_uint32_t *yres,
			     l4_uint32_t *bits_per_pixel,
			     l4_uint32_t *bytes_per_pixel,
			     l4_uint32_t *bytes_per_line,
			     l4_uint32_t *flags,
			     l4_uint32_t *xtxt,
			     l4_uint32_t *ytxt,
			     CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  
  *g_mode          = vc->gmode;
  *xres            = vc->client_xres;
  *yres            = vc->client_yres;
  *bits_per_pixel  = vc->bpp;
  *bytes_per_pixel = vc->bytes_per_pixel;
  *bytes_per_line  = vc->bytes_per_line;
  *flags           = vc->flags;
  *xtxt            = FONT_XRES;
  *ytxt            = FONT_YRES;
  return 0;
}

/**
 * Get Get RGB pixel values.
 * \param  _dice_corba_obj  IDL request structure
 * \retval red_offs         offset of red value in pixel
 * \retval red_len          length of red value in pixel
 * \retval green_offs       offset of green value in pixel
 * \retval green_len        length of green value in pixel
 * \retval blue_offs        offset of blue value in pixel
 * \retval blue_len         length of blue value in pixel
 * \retval _dice_corba_env  IDL exception (unused)
 * \return 0                success */
long
con_vc_graph_get_rgb_component(CORBA_Object _dice_corba_obj,
                               l4_uint32_t *red_offs,
                               l4_uint32_t *red_len,
                               l4_uint32_t *green_offs,
                               l4_uint32_t *green_len,
                               l4_uint32_t *blue_offs,
                               l4_uint32_t *blue_len,
                               CORBA_Server_Environment *_dice_corba_env)
{
  *red_offs   = VESA_RED_OFFS;
  *red_len    = VESA_RED_SIZE;
  *green_offs = VESA_GREEN_OFFS;
  *green_len  = VESA_GREEN_SIZE;
  *blue_offs  = VESA_BLUE_OFFS;
  *blue_len   = VESA_BLUE_SIZE;

  return 0;
}

long 
con_vc_graph_mapfb_component(CORBA_Object _dice_corba_obj,
                             unsigned long fb_offset,
			     l4_snd_fpage_t *page,
			     unsigned long *page_offset,
			     CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  l4_addr_t base = l4_trunc_superpage(vis_vmem);
  l4_offs_t offs = (l4_addr_t)vis_vmem - base;

  /* deliver offset in any case! */
  *page_offset = fb_offset == 0 ? offs : 0;

  /* don't allow a client in the background to map the framebuffer! */
  if (!l4_tasknum_equal(*_dice_corba_obj, vc_partner_l4id))
    {
      /* scan clients */
      int i;
      
      for (i=0; i<CONFIG_MAX_CLIENTS; i++)
        if (l4_tasknum_equal(*_dice_corba_obj, vc->clients[i]))
          break;

      if (i >= CONFIG_MAX_CLIENTS)
        {
          /* not found */
          /* XXX No support from DICE to prevent setting bit 1 of the send message
           *     descriptor. Therefore we invalidate the flexpage */
          LOG_printf("mapfb: not allowed to map FB. Currently allowed:\n");
          for (i=0; i<CONFIG_MAX_CLIENTS; i++)
              if (!l4_is_invalid_id(vc->clients[i]))
                  LOG_printf("  "l4util_idfmt"\n", l4util_idstr(vc->clients[i]));
          page->snd_base  = 0;
          page->fpage.raw = 0;
          return -L4_EPERM;
        }
    }

  if ((gr_vmem + fb_offset + L4_SUPERPAGESIZE > gr_vmem_maxmap) ||
      (fb_offset % L4_SUPERPAGESIZE))
    {
      /* XXX No support from DICE to prevent setting bit 1 of the send message
       *     descriptor. Therefore we invalidate the flexpage */
      page->snd_base  = 0;
      page->fpage.raw = 0;
      return -L4_EINVAL_OFFS;
    }

  page->snd_base = 0;
  page->fpage    = l4_fpage(base+fb_offset, L4_LOG2_SUPERPAGESIZE,
		            L4_FPAGE_RW, L4_FPAGE_MAP);
  vc->fb_mapped = 1;
  update_id = 1;

  return 0;
}


long 
con_vc_ev_sflt_component(CORBA_Object _dice_corba_obj,
			 unsigned long filter,
			 CORBA_Server_Environment *_dice_corba_env)
{
  return -CON_ENOTIMPL;
}

long 
con_vc_ev_gflt_component(CORBA_Object _dice_corba_obj,
			 unsigned long *filter,
			 CORBA_Server_Environment *_dice_corba_env)
{
  return -CON_ENOTIMPL;
}

/**
 * Fill rectangular area of virtual framebuffer with color. */
long 
con_vc_pslim_fill_component(CORBA_Object _dice_corba_obj,
			    const l4con_pslim_rect_t *rect,
			    l4con_pslim_color_t color,
			    CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  
  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  vc_fill(vc, 1, (l4con_pslim_rect_t*)rect, color);
  /* wait for any pending acceleration operation before return because the 
   * user has direct access to the framebuffer */
  if (vc->fb_mapped)
    vc->do_sync();
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}

/**
 * Copy rectangular area of virtual framebuffer to (dx,dy). */
long 
con_vc_pslim_copy_component(CORBA_Object _dice_corba_obj,
			    const l4con_pslim_rect_t *rect,
			    l4_int16_t dx,
			    l4_int16_t dy,
			    CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;
  
  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_copy(vc, 1, (l4con_pslim_rect_t*)rect, dx, dy);
  /* wait for any pending acceleration operation before return because the 
   * user has direct access to the framebuffer */
  if (vc->fb_mapped)
    vc->do_sync();
  l4lock_unlock(&vc->fb_lock);

  return 0;
}

/**
 * Set rectangular area of virtual framebuffer with foreground and background
 * color mask in bitmap. */
long 
con_vc_pslim_bmap_component(CORBA_Object _dice_corba_obj,
			    const l4con_pslim_rect_t *rect,
			    l4con_pslim_color_t fg_color,
			    l4con_pslim_color_t bg_color,
			    const l4_uint8_t* bmap,
			    long bmap_size,
			    l4_uint8_t bmap_type,
			    CORBA_Server_Environment *_dice_corba_env)
{
  void *map = (void*)bmap;
  struct l4con_vc *vc = (struct l4con_vc*)(_dice_corba_env->user_data);
  
  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  convert_color(vc, &fg_color);
  convert_color(vc, &bg_color);

  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_bmap(vc, 1, (l4con_pslim_rect_t*)rect, 
	       fg_color, bg_color, map, bmap_type);
  l4lock_unlock(&vc->fb_lock);

  return 0;
}

/**
 * Set rectangular area of virtual framebuffer with color in pixelmap. */
long 
con_vc_pslim_set_component(CORBA_Object _dice_corba_obj,
			   const l4con_pslim_rect_t *rect,
			   const l4_uint8_t* pmap,
			   long pmap_size,
			   CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  void *map = (void*)pmap;

  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_set(vc, 1, (l4con_pslim_rect_t*)rect, map);
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}

/**
 * Convert pixmap from YUV to RGB color space, scale and set rectangular area
 * of virtual framebuffer. */
long
con_vc_pslim_cscs_component (CORBA_Object _dice_corba_obj,
                             const l4con_pslim_rect_t *rect,
                             const unsigned char *y,
                             int y_l,
                             const unsigned char *u,
                             int u_l,
                             const unsigned char *v,
                             int v_l,
                             long yuv_type,
                             char scale,
                             CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);

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
	  pslim_cscs(vc, 1, (l4con_pslim_rect_t*)rect,
		      (void*)y, (void*)v, (void*)u,
		      yuv_type, 1);
	  break;
	case pSLIM_CSCS_PLN_YV12:
	  pslim_cscs(vc, 1, (l4con_pslim_rect_t*)rect,
		      (void*)y, (void*)u, (void*)v,
		      yuv_type, 1);
	  break;
	}
    }
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}

/**
 * Set rectangular area of virtual framebuffer with color in pixelmap. */
long
con_vc_puts_component (CORBA_Object _dice_corba_obj,
                       const char *s,
                       int len,
                       short x,
                       short y,
                       l4con_pslim_color_t fg_color,
                       l4con_pslim_color_t bg_color,
                       CORBA_Server_Environment *_dice_corba_env)
{
  long ret;
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  
  l4lock_lock(&vc->fb_lock);
  ret = vc_puts(vc, 1, s, len, x, y, fg_color, bg_color);
  l4lock_unlock(&vc->fb_lock);

  return ret;
}

/**
 * Set rectangular area of virtual framebuffer with color in pixelmap. */
long
con_vc_puts_attr_component (CORBA_Object _dice_corba_obj,
                            const short *s,
                            int strattr_size,
                            short x,
                            short y,
                            CORBA_Server_Environment *_dice_corba_env)
{
  int i, j;
  l4_uint16_t* str = (l4_uint16_t*) s;
  
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    {
      for(i=0; i<strattr_size; i+=2)
	{
	  const int c = *str++;
	  const l4con_pslim_color_t fgc = vc->color_tab[(c & 0x0F00) >> 8];
	  const l4con_pslim_color_t bgc = vc->color_tab[(c & 0xF000) >> 12];
	  
	  if ((c & 0xFF) == ' ')
	    {
	      /* optimization: collect spaces */
	      l4con_pslim_rect_t rect;
	      
	      for (j=1; (*str == c) && (i<strattr_size-2); i+=2, j++, str++)
		;
	      rect = (l4con_pslim_rect_t) { x, y, j*FONT_XRES, FONT_YRES };
	      pslim_fill(vc, 1, &rect, bgc);
	      x += j*FONT_XRES;
	    }
	  else
	    {
	      l4con_pslim_rect_t rect = { x, y, FONT_XRES, FONT_YRES };
	      
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


long 
con_vc_direct_update_component(CORBA_Object _dice_corba_obj,
			       const l4con_pslim_rect_t *rect,
			       CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
  
  if ((vc->mode & CON_OUT)==0)
    return -CON_EPERM;

  if (vc->fb_mapped)
    {
      if (hw_accel.caps & ACCEL_POST_DIRTY)
	{
	  /* notify the hardware that there was something changed. */
	  if (vc->do_drty)
	    vc->do_drty(rect->x, rect->y, rect->w, rect->h);
	  return 0;
	}
      else
	{
	  static int bug;
	  if (!bug)
	    {
	      LOG_printf("fb mapped and post dirty probably not necessary "
		         "by "l4util_idfmt"\n", l4util_idstr(*_dice_corba_obj));
	      bug++;
	    }
	}
    }

  if (vc->vfb == 0)
    {
      LOG_printf("no vfb set\n");
      return -CON_EPERM;
    }
  
  /* need fb_lock for drawing */
  l4lock_lock(&vc->fb_lock);
  if(vc->fb != 0)
    pslim_set(vc, 1, (l4con_pslim_rect_t*)rect, 0 /* use mapped vfb */);
  l4lock_unlock(&vc->fb_lock);
  
  return 0;
}


long
con_vc_direct_setfb_component(CORBA_Object _dice_corba_obj,
			      const l4dm_dataspace_t *data_ds,
			      CORBA_Server_Environment *_dice_corba_env)
{
  int error;
  l4_size_t size;

  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);

  if (vc->vfb_in_server)
    {
      LOG("Virtual framebuffer allocated by server -- direct_setfb nonsense");
      return -L4_EINVAL;
    }

  if (vc->vfb)
    {
      LOG("Virtual framebuffer already mapped -- direct_setfb nonsense");
      return -L4_EINVAL;
    }

  if ((error = l4dm_mem_size((l4dm_dataspace_t*)data_ds, &size)))
    {
      LOG("Error %d requesting size of data_ds", error);
      return -L4_EINVAL;
    }
  
  if ((error = l4rm_attach((l4dm_dataspace_t*)data_ds, size, 0, 
			   L4DM_RO | L4RM_MAP | L4RM_SUPERPAGE_ALIGNED,
                           (void*)&vc->vfb)))
    {
      LOG("Error %d attaching data_ds", error);
      return -L4_EINVAL;
    }

  LOG_printf("Mapped client FB to %08lx size %08zx\n",
            (l4_addr_t)vc->vfb, size);

  return 0;
}

/**
 * Convert pixmap from YUV to RGB color space, scale and set rectangular area
 * of virtual framebuffer. */
long 
con_vc_stream_cscs_component(CORBA_Object _dice_corba_obj,
			     const l4con_pslim_rect_t *rect_src,
			     const l4con_pslim_rect_t *rect_dst,
			     l4_uint8_t yuv_type,
			     l4_snd_fpage_t *buffer,
			     unsigned long *offs_y,
			     unsigned long *offs_u,
			     unsigned long *offs_v,
			     CORBA_Server_Environment *_dice_corba_env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(_dice_corba_env->user_data);
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
  config.dest   = (vidix_rect_t){ rect_dst->x, rect_dst->y,
				  rect_dst->w, rect_dst->h, { 0, 0, 0 } };

  if (config.dest.x > vc->client_xres-32)
    config.dest.x = vc->client_xres-32;
  if (config.dest.y > vc->client_yres-32)
    config.dest.y = vc->client_yres-32;
  if (config.dest.w < 32)
    config.dest.w = 32;
  else if (config.dest.w+config.dest.x > vc->client_xres)
    config.dest.w = vc->client_xres-config.dest.x;
  if (config.dest.h < 32)
    config.dest.h = 32;
  else if (config.dest.h+config.dest.y > vc->client_yres)
    config.dest.h = vc->client_yres-config.dest.y;

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
      l4con_pslim_rect_t rect = { config.dest.x, config.dest.y, 
				  config.dest.w, config.dest.h };
      l4con_pslim_color_t pink = { 0x00FF00FF };

      l4lock_lock(&vc->fb_lock);

      convert_color(vc, &pink);
      pslim_fill(vc, 0, &rect, pink);

      vc->logo_x = config.dest.x + 20;
      vc->logo_y = config.dest.y + 20;

      l4lock_unlock(&vc->fb_lock);
    }

  printf("Opening cscs stream %dx%d => %dx%d\n",
         config.src.w, config.src.h, config.dest.w, config.dest.h);
  buffer->fpage = l4_fpage((l4_addr_t)config.dga_addr, 
			    l4util_log2(config.frame_size),
			   L4_FPAGE_RW, L4_FPAGE_MAP);

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

/**
 * inits receive strings (replaces call to flick_server_set_rcvstring) */
void
vc_init_rcvstring(int nb, l4_umword_t* addr, l4_umword_t* size,
		  CORBA_Server_Environment *env)
{
  struct l4con_vc *vc = (struct l4con_vc *)(env->user_data);
  
  if (nb==0)
    {
      *addr = (l4_umword_t)vc->sbuf1;
      *size = vc->sbuf1_size;
    }
  else if (nb == 1)
    {
      *addr = (l4_umword_t)vc->sbuf2;
      *size = vc->sbuf2_size;
    }
  else if (nb == 2)
    {
      *addr = (l4_umword_t)vc->sbuf3;
      *size = vc->sbuf3_size;
    }
  else
    Panic("unknown string init (%d)", nb);
}

#ifdef ARCH_x86

void vc_pf_entry(void);
asm ("vc_pf_entry:		\n\t"
     "subl  $4, %esp		\n\t"
     "pusha			\n\t"
     "push  32(%esp)		\n\t"
     "call  vc_pf_handler	\n\t"
     "addl  $4, %esp		\n\t"
     "popa			\n\t"
     "addl  $8, %esp		\n\t"
     "iret			\n\t");

/**
 * Pagefault handler for console threads. A pagefault may occur if the client
 * sends a flexpage which is too small.
 */
static L4_STICKY(void)
vc_pf_handler(l4_addr_t pf_addr)
{
  int i;
  l4_threadid_t me = l4thread_l4_id(l4thread_myself());

  for (i=1; i<=MAX_NR_L4CONS; i++)
    {
      if (l4_thread_equal(me, vc[i]->vc_l4id))
	{
	  printf("vc[%d]: page fault at "l4_addr_fmt" -- closing\n", i, pf_addr);

	  vc_close(vc[i]);
	  if (vc[i]->mode == CON_CLOSING)
	    {
	      /* mark vc as free */
	      vc[i]->mode = CON_CLOSED;
	      l4thread_exit();
	    }
	}
    }

  printf("page fault in unknown vc (addr="l4_addr_fmt"\n", pf_addr);
  enter_kdebug("stop");
}

/**
 * Setup a PF handler. A client might have passed a dataspace which the console
 * does not have access. In this case, close the vc. */
static void
vc_setup_pf_handler(struct l4con_vc *this_vc)
{
  static struct
    {
      l4util_idt_header_t header;
      l4util_idt_desc_t   desc[15];
    } __attribute__((packed)) idt;

  l4util_idt_init(&idt.header, 15);
  l4util_idt_entry(&idt.header, 14, (void*)&vc_pf_entry);
  l4util_idt_load(&idt.header);

  l4rm_enable_pagefault_exceptions();
}

#endif

/**
 * con_vc - IDL server loop. */
void 
vc_loop(struct l4con_vc *this_vc)
{
  CORBA_Server_Environment env = dice_default_server_environment;
  env.timeout = L4_IPC_SEND_TIMEOUT_0;
  env.user_data = (void*)this_vc;

#ifdef ARCH_x86
  vc_setup_pf_handler(this_vc);
#endif

  l4thread_started(NULL);

  LOG("vc[%d] running as "l4util_idfmt"",
	this_vc->vc_number, l4util_idstr(l4thread_l4_id(l4thread_myself())));

  con_vc_server_loop(&env);
  
  Panic("IDL IPC error occured");
}

void
vc_error(l4_msgdope_t result, CORBA_Server_Environment *env)
{
#if 0
  if (L4_IPC_ERROR(result) == L4_IPC_ENOT_EXISTENT)
    {
      /* application was killed => close vc */
      LOG("Partner thread killed, closing console %d", this_vc->number);
      want_vc = this_vc->vc_number |= 0x1000;
    }
  else
#endif
    LOG("vc error %08lx (%s)",
	L4_IPC_ERROR(result), l4env_strerror(L4_IPC_ERROR(result)));
}
