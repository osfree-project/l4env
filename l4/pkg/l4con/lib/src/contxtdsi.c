/*!
 * \file	con/lib/src/contxtdsi.c
 *
 * \brief	libcontxtdsi client library
 *
 * \author	Mathias Noack <mn3@os.inf.tu-dresden.de>
 *
 */

/* OSKit includes */
#include <stdlib.h>
#include <string.h>

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/names/libnames.h>

/* intern */
#include "internal.h"
#include "evh.h"
#include "send_server.h"


/*****************************************************************************
 *** global variables
 *****************************************************************************/

/**
 * con server thread id
 */
static l4_threadid_t con_l4id;
l4_threadid_t vtc_l4id;		/* l4 thread id of appr. console thread */
int scrbuf_size;		/* size of buffer */

/**
 * DSI stream configuration
 */
dsi_stream_cfg_t config = 
(dsi_stream_cfg_t){120,	        /*!< number of control packets */
		   1};		/*!< max. length of sg_list */

/** 
 * default DSI jcp stream configuration 
 */
static dsi_jcp_stream_t stream_desc = 
(dsi_jcp_stream_t){0,		/* bandwith (byte/s) */
		   0,		/* jitter (ms) */
		   0};		/* packet size */

int __init	= 0;
int accel_flags = 0;
int fn_x, fn_y;

static int
receiver_connect(dsi_component_t *comp, dsi_socket_ref_t *remote)
{
  int ret;
  sm_exc_t _exc;
  
  if (l4_is_nil_id(vtc_l4id))
    return -L4_EINVAL;
  
  ret = con_dsi_vc_connect(vtc_l4id,
			   (con_dsi_socket_t *)&comp->socketref,
			   (con_dsi_socket_t *)remote,
			   &_exc);
  if (ret || (_exc._type != exc_l4_no_exception)) 
    {
      LOGl("connect socket failed (ret %d, exc %d)",
	   ret,_exc._type);
      if (ret)
	return ret;
      else
	return -L4_EIPC;
    }
  return 0;
}


/*****************************************************************************/
/**
 * \brief   Init of contxtdsi library
 * 
 * \param   scrbuf_lines   ... number of additional screenbuffer lines 
 *
 * This is the init-function of libcontxtdsi. It opens a dsi-console, 
 * initialises the DSI sender/reciever and starts the DSI stream.
 */
/*****************************************************************************/
int
contxtdsi_init(int scrbuf_lines)
{
  l4_uint8_t gmode;
  unsigned bits_per_pixel, bytes_per_pixel;
  int xres, yres;
  dsi_component_t sender, receiver;
  l4dm_dataspace_t ctrl, data;
  dsi_stream_t * stream;
  dsi_socket_ref_t sock_ref;
  sm_exc_t _ev;
  
  /* ask for 'con' */
  if (!names_waitfor_name(CON_NAMES_STR, &con_l4id, CONTXT_TIMEOUT))
    {
      LOGl("names failed");
      return -L4_EINVAL;
    }
  
  if (con_if_dsi_openqry(con_l4id, 
			 (con_threadid_t*) &vtc_l4id, 
			 CON_NOVFB,
			 &_ev))
    {
      LOGl("dsi_openqry failed");
      return -L4_EINVAL;
    }
  
  evh_init();
   
  if(con_dsi_vc_graph_gmode(vtc_l4id, &gmode, &xres, &yres,
			    &bits_per_pixel, &bytes_per_pixel, 
			    &accel_flags, &fn_x, &fn_y, &_ev))
    {
      LOGl("gmode failed");
      return -L4_EINVAL;
    }
  
  vtc_cols = xres / fn_x;
  vtc_lines = yres / fn_y;
  
  /* init DSI library */
  dsi_init();
  
  send_server_init();
  
  /* size of whole screen buffer */
  scrbuf_size = (vtc_lines + scrbuf_lines) * vtc_cols;
  
  /* open send socket */
  if(send_server_open(&stream_desc, &config, 
		      &sock_ref, &ctrl, &data))
    {
      LOGl("sender open failed");
      return -L4_EINVAL;
    }
  
  /* setup send component descriptor */
  memcpy(&sender.socketref, &sock_ref, sizeof(dsi_socket_ref_t));
  sender.connect = send_server_connect;
  sender.start = send_server_start;
  sender.stop  = NULL;
  sender.close = NULL;
  
  LOG("sender ready!"); 
    
  /* init reciever */
  con_dsi_vc_bmap_open(vtc_l4id, 
		       (con_dsi_jcp_stream_t *) &stream_desc, 
		       (con_dsi_cfg_t *) &config,
		       (con_dataspace_t *) &ctrl, 
		       (con_dataspace_t *) &data, 
		       (con_dsi_socket_t *) &sock_ref,
		       &_ev);
  
  memcpy(&receiver.socketref, &sock_ref, sizeof(dsi_socket_ref_t));
  receiver.connect = receiver_connect;
  receiver.start = NULL;
  receiver.stop = NULL;
  receiver.close = NULL;
  
  LOG("reciever ready!");
  
  if (con_dsi_vc_smode(vtc_l4id, 
		       CON_INOUT,
		       (con_threadid_t*) &evh_l4id,
		       &_ev))
    {
      LOGl("smode failed");
      return -L4_EINVAL;
    }
  
  if(dsi_stream_create(&sender, &receiver, ctrl, data, &stream))
    {
      LOGl("create stream failed");
      return -L4_EINVAL;
    }
  
  if(dsi_stream_start(stream)) 
    {
      LOGl("start stream failed");
      return -L4_EINVAL;
    }
  
  LOGl("stream started");
  
  __init = 1;
  
  _redraw();
  
  return 0;
}

/*****************************************************************************/
/**
 * \brief   close contxtdsi library 
 * 
 * \return  0 on success (close a dsi-console)
 *          PANIC otherwise
 *
 * Close the libcontxtdsi console.
 */
/*****************************************************************************/
int 
contxtdsi_close()
{
  sm_exc_t _ev;

  return con_vc_close(vtc_l4id, &_ev) ;
}

