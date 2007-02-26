/*!
 * \file	con/lib/src/contxt.c
 *
 * \brief	libcontxt client library
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 */

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <string.h>

/* intern */
#include "internal.h"
#include "evh.h"

/*****************************************************************************
 *** global variables
 *****************************************************************************/

/**
 * Con server thread id
 */
static l4_threadid_t con_l4id;

int __init	 = 0;	/* 1: contxt_init done */
int sb_lines     = 0;	/* number of lines in scrbuf */
int accel_flags  = 0;	/* helps to decide if we want to scroll or redraw */
int keylist_head = 0;	/* next char to write to keylist */
int keylist_tail = 0;	/* next char to read from keylist */
int keylist[CONTXT_KEYLIST_SIZE];  /* keyboard buffer */
int fn_x, fn_y;		/* font size x, y */
l4_uint8_t *vtc_scrbuf;	/* screen buffer */


l4_threadid_t vtc_l4id;	/* l4 thread id of appropriate console thread */


//*****************************************************************************/
/**
 * \brief   Init of contxt library
 * 
 * \param   max_sbuf_size  ... max IPC string buffer
 * \param   scrbuf_lines   ... number of additional screenbuffer lines 
 *
 * This is the init-function of libcontxt. It opens a console and allocates 
 * the screen history buffer.
 */
/*****************************************************************************/
int
contxt_init(long max_sbuf_size, int scrbuf_lines)
{
  l4_uint8_t gmode;
  unsigned bits_per_pixel, bytes_per_pixel, bytes_per_line;
  int xres, yres;
  sm_exc_t _ev;
  int prio;
  
  /* ask for 'con' */
  if (!names_waitfor_name(CON_NAMES_STR, &con_l4id, CONTXT_TIMEOUT))
    {
      LOGl("names failed");
      return -L4_EINVAL;
    }
  
  if ((prio = l4thread_get_prio(l4thread_myself())) < 0)
    prio = L4THREAD_DEFAULT_PRIO;

  if (con_if_openqry(con_l4id,
		     max_sbuf_size, 0, 0, prio,
		     (con_threadid_t*) &vtc_l4id,
		     CON_NOVFB, 
		     &_ev)
      || _ev._type != exc_l4_no_exception)
    {
      LOGl("openqry failed (exc=%d)", _ev._type);
      return -L4_EINVAL;
    }

  if (evh_init() < 0)
    return -L4_EINVAL;
  
  if (con_vc_smode(vtc_l4id, 
		   CON_INOUT, 
 		   (con_threadid_t*) &evh_l4id,
	 	   &_ev)
      || _ev._type != exc_l4_no_exception)
    {
      LOGl("smode failed (exc=%d)", _ev._type);
      return -L4_EINVAL;
    }
  
  if(con_vc_graph_gmode(vtc_l4id, &gmode, &xres, &yres,
			&bits_per_pixel, &bytes_per_pixel,
			&bytes_per_line, &accel_flags, 
			&fn_x, &fn_y, &_ev)
      || _ev._type != exc_l4_no_exception)
    {
      LOGl("gmode failed (exc=%d)", _ev._type);
      return -L4_EINVAL;
    }

  vtc_cols  = xres / fn_x;
  vtc_lines = yres / fn_y;
  sb_lines  = vtc_lines + scrbuf_lines;

   LOG("%dx%d, cols:%d, lines:%d, sb_lines:%d",
       xres, yres, vtc_cols, vtc_lines, sb_lines);

  if (!(vtc_scrbuf = (l4_uint8_t *) (l4dm_mem_allocate_named(sb_lines * vtc_cols,
							 0, "contxt buffer"))))
    {
      LOGl("no mem for vtc_scrbuf (%dkB)", (sb_lines*vtc_cols) / 1024);
      return -L4_ENOMEM;
    }
  memset(vtc_scrbuf, ' ', sb_lines * vtc_cols);

  __init = 1;
  
  _redraw();
  
  return 0;
}

/*****************************************************************************/
/**
 * \brief   close contxt library
 * 
 * \return  0 on success (close a console)
 *          PANIC otherwise
 *
 * Close the libcontxt console.
 */
/*****************************************************************************/
int 
contxt_close()
{
  sm_exc_t _ev;
  
  return con_vc_close(vtc_l4id, &_ev);
}

