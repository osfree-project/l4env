/* $Id$ */
/**
 * \file	con/server/include/con.h
 * \brief	some global structures
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _CON_H
#define _CON_H

/* L4 includes */
#include <l4/l4con/l4con.h>
#include <l4/sys/types.h>
#include <l4/lock/lock.h>

/* local includes */
#include "con_config.h"

#define MAX_NR_L4CONS	(CONFIG_MAX_VC+1)	/* number of vc's,
						 * consider master=0 */

struct l4con_vc;

typedef void (*pslim_copy_fn)(struct l4con_vc*, int sx, int sy,
			      int width, int height, int dx, int dy);
typedef void (*pslim_fill_fn)(struct l4con_vc*, int sx, int sy,
			      int width, int height, unsigned color);
typedef void (*pslim_sync_fn)(void);
typedef void (*pslim_drty_fn)(int x, int y, int width, int height);

struct l4con_vc 
{
  l4_uint8_t vc_number;		 /* which vc is it */
  l4_uint8_t mode;		 /* IN, OUT, INOUT, USED, ... */
  l4_threadid_t vc_l4id;	 /* local pSLIM handler */
   
  void *sbuf1;			 /* 1st buffer for "indirect strings" */
  void *sbuf2;			 /* 2nd buffer for "indirect strings" */
  void *sbuf3;			 /* 3rd buffer for "indirect strings" */
  l4_uint32_t sbuf1_size;        /* size of 1st string buffer */
  l4_uint32_t sbuf2_size;	 /* size of 2nd string buffer */
  l4_uint32_t sbuf3_size;	 /* size of 3rd string buffer */

  /* vfb */
  l4_uint8_t  gmode;		 /* graphics mode */
  l4_uint8_t  bpp;		 /* bits per pixel */
  l4_uint32_t user_xres, user_yres; /* screen dimensions (user visible) */
  l4_uint32_t user_xofs, user_yofs; /* offsets of user vfb into physical fb */
  l4_uint32_t xres, yres;	 /* physical dimensions of fb */
  l4_uint32_t xofs, yofs;
  l4_uint32_t logo_x, logo_y;
  l4_uint32_t bytes_per_pixel;	 /* bytes per pixel */
  l4_uint32_t bytes_per_line;	 /* bytes per line */
  l4_uint32_t flags;		 /* miscellaneous flags */
		
  l4_uint8_t *vfb;		 /* begin of vfb in memory */
  l4_uint8_t *fb;		 /* whether vfb or h/w */

  l4_uint8_t vfb_used;           /* vfb mode (vfb used or not) */
  l4_uint8_t fb_mapped;		 /* framebuffer mapped to client */
  l4_uint32_t vfb_size;		 /* size of vfb; depends on g_mode */
  l4lock_t fb_lock;		 /* thread is `drawing' - mutex for fb */
  const l4con_pslim_color_t *color_tab;
  
  l4_taskid_t vc_partner_l4id;	 /* partner task (owner of console) */
  
  /* events */
  l4_uint8_t ev_mode;		 /* event filter */
  
  l4_threadid_t ev_partner_l4id; /* input event handler */

  pslim_copy_fn do_copy;	 /* current function for fb_copy */
  pslim_fill_fn do_fill;	 /* current function for fb_clean */
  pslim_sync_fn do_sync;	 /* current function for sync acceleration */
  pslim_drty_fn do_drty;	 /* function for marking framebuffer areas 
				  * as dirty after direct framebuffer access */
};

#endif /* !_CON_H */
