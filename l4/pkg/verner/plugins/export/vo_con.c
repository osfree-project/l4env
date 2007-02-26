/*
 * \brief   Video specific export plugin using L4con for VERNER's sync component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/bitops.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>

/* libc */
#include <stdlib.h>		/*atoi */

/* DOpE */
#include <l4/con/l4con.h>
#include <l4/con/con-client.h>

/* verner */
#include "arch_globals.h"
/* configuration */
#include "verner_config.h"

/* Local includes */
#include "vo_con.h"

/* global vars */
static l4_threadid_t vc_l4id;			/* partner VC */
static l4_uint32_t xres, yres;
static int stream_cscs = 0; /* using stream or pslim rendering */

/*
 * the following function were shamelessly taken from smart_mpeg
 * BUT slidly modified !
 */
#define FPAGE_SIZE	(1024*1024)

static l4_uint32_t stream_y, stream_u, stream_v;
static l4_addr_t stream_window;
static l4_uint32_t yuv_type;

static void (*yuv2rgb_render) (const unsigned char *py,
		   const unsigned char *pu,
		   const unsigned char *pv,
		   int h_size, int v_size,
		   int y_stride, int uv_stride);

/* Slow transport using pSLIM cscs (kernel string copy). We copy strides
 * of two lines to decrease depending size of receive buffer at console
 * side. */
static void
l4con_render_pslim(const unsigned char *py,
		   const unsigned char *pu,
		   const unsigned char *pv,
		   int h_size, int v_size,
		   int y_stride, int uv_stride);
/* Render picture to graphics cards which support the YV12 planar format
 * (Matrox G400, ATI Mach64). We copy three separate planes (Y, U, V). 
 * Format uses effective 12 bits/pixel. */
static void
l4con_render_stream_YV12(const unsigned char *py,
			 const unsigned char *pu,
     			 const unsigned char *pv,
     			 int h_size, int v_size, 
			 int y_stride, int uv_stride);
/* Render picture to graphics cards which support the YUY2 packed format
 * (newer S3 Savage). Format uses effective 16 bits/pixel: Y0|U0|Y1|U1 */
static void
l4con_render_stream_YUY2(const unsigned char *py,
			 const unsigned char *pu,
			 const unsigned char *pv,
			 int h_size, int v_size, 
			 int y_stride, int uv_stride);
static int l4con_render_stream_init(int width, int height, int aspect);
static void l4con_render_stream_done(void);


/*
 * init 
 */ 
int
vo_con_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  l4con_pslim_rect_t rect;
  CORBA_Environment _env = dice_default_environment;
  l4_threadid_t con_l4id;			/* con at names */
  l4_threadid_t dummy_l4id;
  l4_uint32_t fn_x, fn_y;
  l4_uint32_t bits_per_pixel;
  l4_uint32_t bytes_per_pixel;
  l4_uint32_t bytes_per_line;
  l4_uint32_t accel_flags;
  l4_threadid_t my_l4id;			/* it's me */
  l4_uint8_t gmode;

  int ret;

  if ((info->vi.xdim == 0) || (info->vi.ydim == 0))
  {
    LOG_Error ("invalid stream parameter");
    return -L4_ENOTSUPP;
  }

  /* get info about con */
  my_l4id = l4thread_l4_id( l4thread_myself() );

  /* ask for 'con' (timeout = 5000 ms) */
  if (names_waitfor_name(CON_NAMES_STR, &con_l4id, 5000) == 0) 
  {
      LOG_Error("PANIC: %s not registered at names", CON_NAMES_STR);
      return -L4_ENOTFOUND;
  }

  /* open con */
  if (con_if_openqry_call(&con_l4id, 
#warning check these values
#if 0
		    8192,2048,2048,
#else		    
		    (info->vi.xdim*info->vi.ydim),
		    (info->vi.xdim*info->vi.ydim)/4,
		    (info->vi.xdim*info->vi.ydim)/4,
#endif
		     L4THREAD_DEFAULT_PRIO,
	  	     &vc_l4id, 
  		     CON_VFB,
		     &_env))
  {
    LOG_Error("Ouch, open vc failed");
    return -L4_EUNKNOWN;
  }

  /* output only mode */
  if (con_vc_smode_call(&vc_l4id, CON_OUT, &dummy_l4id, &_env))
  {
    LOG_Error("Ouch, setup vc failed");
    return -L4_EUNKNOWN;
  }

  /* get console params */
  if (con_vc_graph_gmode_call(&vc_l4id, &gmode, &xres, &yres,
			 &bits_per_pixel, &bytes_per_pixel,
			 &bytes_per_line, &accel_flags, 
			 &fn_x, &fn_y, &_env))
  {
    LOG_Error("Ouch, graph_gmode failed");
    return -L4_ENOTSUPP;
  }
  
  /* check for hw accel in backend */
  if (accel_flags & (L4CON_STREAM_CSCS_YV12|L4CON_STREAM_CSCS_YUY2))
      {
	  /* console supports hardware accelerated yuv2rgb conversion */
	  stream_cscs = 1;
	  printf("using HW accel!\n");
	  if (accel_flags & L4CON_STREAM_CSCS_YV12)
	    {
	      /* use fast planar format without translation */
	      yuv2rgb_render = l4con_render_stream_YV12;
	      yuv_type = pSLIM_CSCS_PLN_YV12;
	    }
	  else
	    {
	      /* translate into packed format (needs 33% more data/frame!) */
	      yuv2rgb_render = l4con_render_stream_YUY2;
	      yuv_type = pSLIM_CSCS_PCK_YUY2;
	    }
          l4con_render_stream_init(info->vi.xdim,info->vi.ydim,100);
      }
  else
      {
	/* slow variant using pslim */
	stream_cscs = 0;
	yuv2rgb_render = l4con_render_pslim;
      }

  
  /* clear screen */
  rect.x = 0; rect.y = 0;
  rect.w = xres; rect.h = yres;
  ret = con_vc_pslim_fill_call(&vc_l4id, &rect, 0x000000/*black*/, &_env);
  if (ret) 
    {
      LOG_Error("pslim_fill returned %i", ret);
      return -1;
    }
  
  /* done */
  return 0;
}

int
vo_con_commit (plugin_ctrl_t * attr)
{
  /* done */
  return 0;
}

int
vo_con_close (plugin_ctrl_t * attr)
{
  CORBA_Environment _env = dice_default_environment;
  
  /* close stream render */
  if(stream_cscs)
  {
    l4con_render_stream_done();
    stream_cscs = 0;
  }
  
  /* close con */
  if (con_vc_close_call(&vc_l4id, &_env))
  {
    LOG_Error("Ouch, close vc failed?!");
    return -L4_EUNKNOWN;
  }

  /* done */
  return 0;
}


int
vo_con_step (plugin_ctrl_t * attr, void *addr)
{

  frame_ctrl_t *frameattr = (frame_ctrl_t *) addr;

  /* check if valid */
  if (attr->mode != PLUG_MODE_EXPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (!addr)
  {
    LOG_Error ("no data to export");
    return -L4_ENOHANDLE;
  }

#warning ADD REACTION TO RESET ATTR
  if (is_reconfigure_point (frameattr->keyframe))
  {
    /* should we close and reopen con ? */
        Panic ("reset_attr_point detected - not working yet - now I'll crash!");
  }	
  
  yuv2rgb_render((unsigned char*)(addr+sizeof(frame_ctrl_t)), 
		 (unsigned char*)(addr+sizeof(frame_ctrl_t) + (frameattr->vi.xdim*frameattr->vi.ydim)),
		 (unsigned char*)(addr+sizeof(frame_ctrl_t) + (frameattr->vi.xdim*frameattr->vi.ydim) + (frameattr->vi.xdim*frameattr->vi.ydim)/4),
		 (int) (frameattr->vi.xdim), (int) (frameattr->vi.ydim),
		 (int) ((frameattr->vi.xdim*frameattr->vi.ydim)/2),
		 (int) ((frameattr->vi.xdim*frameattr->vi.ydim)/4));
 
  /* done */
  return 0;
}


/*
 * the following code was shamelessly taken from smart_mpeg
 */
 

/* Slow transport using pSLIM cscs (kernel string copy). We copy strides
 * of two lines to decrease depending size of receive buffer at console
 * side. */
static void
l4con_render_pslim(const unsigned char *py,
		   const unsigned char *pu,
		   const unsigned char *pv,
		   int h_size, int v_size,
		   int y_stride, int uv_stride)
{
  //int i;
  l4con_pslim_rect_t rect;
  // = { 50, 50, h_size, 2 };
  long y_len, u_len, v_len;

  CORBA_Environment env = dice_default_environment;

  /* setup initial vfb area info */
  rect.x = (xres-h_size)/2; 
  rect.y = (yres-v_size)/2;
  rect.w = h_size;
  rect.h = v_size;

  y_len = 2*y_stride;
  u_len = uv_stride;
  v_len = uv_stride;

#warning readd displaying a few lines and not the whole frame
//  v_size /= 2;
//  rect.h = 50;

//  for (i=0; i<v_size; i++)
    {
      if (con_vc_pslim_cscs_call(&vc_l4id, &rect, 
	    (char*)py, y_len, 
	    (char*)pu, u_len, 
	    (char*)pv, v_len, 
	    pSLIM_CSCS_PLN_YV12, 1, &env))
	printf("Error doing pslim_cscs (exc=%d)\n", env.major);
      if (env.major != CORBA_NO_EXCEPTION)
	printf("Error doing pslim_cscs (exc=%d)\n", env.major);
/*
      rect.y += 2;
      py += y_len;
      pu += u_len;
      pv += v_len;
*/      
    }

}
 
/* Render picture to graphics cards which support the YV12 planar format
 * (Matrox G400, ATI Mach64). We copy three separate planes (Y, U, V). 
 * Format uses effective 12 bits/pixel. */
static void
l4con_render_stream_YV12(const unsigned char *py,
			 const unsigned char *pu,
     			 const unsigned char *pv,
     			 int h_size, int v_size, 
			 int y_stride, int uv_stride)
{
  int i;
  unsigned sy=stream_y, su=stream_u, sv=stream_v;
  unsigned sty, stuv;

  sty=(h_size+31)&~31;
  stuv=(h_size/2+15)&~15;
  for (i=0; i<v_size; i++)
    {
      memcpy((void*)sy, py, h_size);
      py += h_size;
      sy += sty;
    }
  for (i=0; i<v_size/2; i++)
    {
      memcpy((void*)su, pu, h_size/2);
      memcpy((void*)sv, pv, h_size/2);
      pu += h_size/2;
      pv += h_size/2;
      su += stuv;
      sv += stuv;
    }
}

/* Render picture to graphics cards which support the YUY2 packed format
 * (newer S3 Savage). Format uses effective 16 bits/pixel: Y0|U0|Y1|U1 */
static void
l4con_render_stream_YUY2(const unsigned char *py,
			 const unsigned char *pu,
			 const unsigned char *pv,
			 int h_size, int v_size, 
			 int y_stride, int uv_stride)
{
  int i,j;
  unsigned sy=stream_y;
  unsigned sty;

  sty=2*((h_size+15)&~15);
  h_size >>= 1;
  for (i=0; i<v_size; i++)
    {
      for (j=0; j<h_size; j++)
	{
	  ((unsigned int*)sy)[j] = py[2*j  ]     | pu[j]<<8
				 | py[2*j+1]<<16 | pv[j]<<24;
	}
      sy += sty;
      py += 2*h_size;
      pu += -(i & 1) & h_size;
      pv += -(i & 1) & h_size;
    }
}

static int
l4con_render_stream_init(int width, int height, int aspect)
{
  int error;
  l4_uint32_t area;
  l4con_pslim_rect_t rect_src = { 0, 0, width, height };
  l4con_pslim_rect_t rect_dst;
  l4_snd_fpage_t snd_fpage;
  CORBA_Environment env = dice_default_environment;

  rect_dst.w = xres;
  if (aspect != 100)
    rect_dst.h = (xres * 100) / aspect;
  else
    rect_dst.h = xres * height / width;

  if (rect_dst.h > yres)
    rect_dst.h = yres;
  if (rect_dst.w > xres)
    rect_dst.w = xres;

  rect_dst.x = (xres  - rect_dst.w) / 2;
  rect_dst.y = (yres - rect_dst.h) / 2;

  if ((error = l4rm_area_reserve(FPAGE_SIZE, L4RM_LOG2_ALIGNED,
				 &stream_window, &area)) < 0)
    {
      printf("Error %d reserving yuv buffer\n", error);
      return error;
    }

  env.rcv_fpage = l4_fpage(stream_window, l4util_log2(FPAGE_SIZE), 0, 0);
  if (con_vc_stream_cscs_call(&vc_l4id,
			 &rect_src, &rect_dst, yuv_type, &snd_fpage,
			 &stream_y, &stream_u, &stream_v,
			 &env)
      || (env.major != CORBA_NO_EXCEPTION))
    {
      printf("Error doing stream_cscs\n");
      return -L4_EINVAL;
    }

  stream_y += stream_window;
  stream_u += stream_window;
  stream_v += stream_window;

  return 0;
}

static void
l4con_render_stream_done(void)
{
  l4_fpage_unmap(l4_fpage(stream_window, l4util_log2(FPAGE_SIZE), 0, 0),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  l4rm_area_release_addr((void*)stream_window);
}


