/* $Id$ */

/*	con/examples/demo1/demo1.c
 *
 *	demonstration server for con
 *
 *	pSLIM_SET ('DROPS CON 2000' - logo)
 */

/* L4 includes */
#include <l4/thread/thread.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

#include <l4/con/l4con.h>
#include <l4/con/con-client.h>

#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/oskit10_l4env/support.h>

#define PROGTAG		"_demo1"

#define MY_SBUF_SIZE	65536

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>

/* local includes */
#include "util.h"
#include "colors.h"
#include "examples_config.h"
#include "bmaps/set.bmap"
#include "pmaps/drops.pmap"

/* internal prototypes */
int clear_screen(void);
int create_logo(void);
int logo(void);

/* global vars */
l4_threadid_t my_l4id;			/* it's me */
l4_threadid_t con_l4id;			/* con at names */
l4_threadid_t vc_l4id;			/* partner VC */
l4_uint16_t *pmap_buf;			/* logo buffer pointer */
l4_uint8_t gmode;
l4_uint32_t xres, yres;
l4_uint32_t fn_x, fn_y;
l4_uint32_t bits_per_pixel;
l4_uint32_t bytes_per_pixel;
l4_uint32_t bytes_per_line;
l4_uint32_t accel_flags;

/******************************************************************************
 * clear_screen                                                               *
 *                                                                            *
 * Nomen est omen.                                                            *
******************************************************************************/
int clear_screen()
{
  int ret = 0;
  char buffer[30];

  con_pslim_rect_t rect;
  l4_strdope_t bmap;
  sm_exc_t _ev;
  
  /* setup initial vfb area info */
  rect.x = 0; rect.y = 0;
  rect.w = xres; rect.h = yres;

  ret = con_vc_pslim_fill(vc_l4id, 
			  (con_pslim_rect_t *) &rect, black, &_ev);
  if (ret) 
    {
      ret2ecodestr(ret, buffer);
      printf("pslim_fill returned %s error\n", buffer);
      return -1;
    }

  /* setup new vfb area info */
  rect.x = xres-84; rect.y = 0;
  rect.w = 88; rect.h = 25;

  /* setup L4 string dope */
  bmap.snd_size = 275;
  bmap.snd_str = (l4_umword_t) set_bmap;
  bmap.rcv_size = 0;

  ret = con_vc_pslim_bmap(vc_l4id,
			  (con_pslim_rect_t *) &rect, 
		  	  black, white, bmap,
			  pSLIM_BMAP_START_MSB, 
			  &_ev);
  if (ret) 
    {
      ret2ecodestr(ret, buffer);
      printf("pslim_bmap returned %s error\n", buffer);
      return -1;
    }
  
  return 0;
}

/******************************************************************************
 * create_logo                                                                *
 *                                                                            *
 * 24 bit picture to 16 bit pmap                                              *
 ******************************************************************************/
int create_logo()
{
  int i;

  /* create 16 bit picture from logo_pmap */
  if (!(pmap_buf = (l4_uint16_t *) malloc(160*120*2)))
    return 1;

  for (i=0; i<160*120; i++) 
    {
      pmap_buf[i] = set_rgb16(logo_pmap[3*i], 
			      logo_pmap[3*i+1], 
			      logo_pmap[3*i+2]);
    }
  
  return 0;
}

/******************************************************************************
 * logo                                                                       *
 *                                                                            *
 * show logo on screen using pSLIM SET                                        *
 ******************************************************************************/
int logo()
{
  int ret = 0, i;
  char buffer[30];

  static l4_int16_t xy_idx[][2] = 
    {
	{ 170,  10}, { 300, 100}, 
	{   0, 360}, { 470, 280},
	{ -50, 200}, { 280, 290},
	{ 540, 400}, { 300, 300},
	{ 330, 350}, { 400,-100}
    };

  con_pslim_rect_t rect;
  l4_strdope_t pmap;
  sm_exc_t _ev;

  /* setup initial vfb area info */
  rect.x = 0; rect.y = 0;
  rect.w = 160; rect.h = 120;

  /* setup L4 string dope */
  pmap.snd_size = 38400;
  pmap.snd_str = (l4_umword_t) pmap_buf;
  pmap.rcv_size = 0;

  for (i=0; i<=10; i++) 
    {
      ret = con_vc_pslim_set(vc_l4id, 
			     (con_pslim_rect_t *) &rect, 
	      		     pmap, 
			     &_ev);
      if (ret) 
	{
	  ret2ecodestr(ret, buffer);
	  printf("pslim_set returned %s error\n", buffer);
	  return -1;
	}

      /* setup new vfb area info */
      rect.x = xy_idx[i][0]; rect.y = xy_idx[i][1];

      l4_sleep(800);
    }

  return 0;
}

/******************************************************************************
 * main                                                                       *
 *                                                                            *
 * Main function                                                              *
 ******************************************************************************/
int main(int argc, char *argv[])
{
  int error = 0;
  l4_threadid_t dummy_l4id = L4_NIL_ID;

  sm_exc_t _ev;

  /* init */
  do_args(argc, argv);
  LOG_init(PROGTAG);
  OSKit_libc_support_init(DEMO1_MALLOC_MAX_SIZE);
  my_l4id = l4thread_l4_id( l4thread_myself() );

  printf("Hello, I'm running as %x.%02x\n",
	 my_l4id.id.task, my_l4id.id.lthread);

  /* ask for 'con' (timeout = 5000 ms) */
  if (names_waitfor_name(CON_NAMES_STR, &con_l4id, 50000) == 0) 
    {
      printf("PANIC: %s not registered at names", CON_NAMES_STR);
      enter_kdebug("panic");
    }

  if (con_if_openqry(con_l4id, MY_SBUF_SIZE, 0, 0,
		     L4THREAD_DEFAULT_PRIO,
		     (con_threadid_t*) &vc_l4id, 
	  	     CON_VFB, &_ev))
    enter_kdebug("Ouch, open vc failed");
  
  if (con_vc_smode(vc_l4id, CON_OUT, (con_threadid_t*)&dummy_l4id, &_ev))
    enter_kdebug("Ouch, setup vc failed");

  if (con_vc_graph_gmode(vc_l4id, &gmode, &xres, &yres,
			 &bits_per_pixel, &bytes_per_pixel,
			 &bytes_per_line, &accel_flags, 
			 &fn_x, &fn_y, &_ev))
    enter_kdebug("Ouch, graph_gmode failed");

  if (create_logo())
    enter_kdebug("Ouch, logo creation failed");

  while (!error) 
    {
      if ((error = clear_screen()))
	enter_kdebug("Ouch, clear_screen failed");
      if ((error = logo()))
	enter_kdebug("Ouch, logo failed");
      l4_sleep(2000);
    }

  if (con_vc_close(vc_l4id, &_ev))
    enter_kdebug("Ouch, close vc failed?!");
  
  printf("Finally closed vc\n");

  printf("Going to bed ...\n");
  l4_sleep(-1);

  return 0;
}
