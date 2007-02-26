/* $Id$ */

/*	con/server/src/dsi_vc.c
 *
 *	virtual console stuff for DSI
 *	ATTENTION: it's multi threaded
 */

/* L4 includes */
#include <l4/con/l4con.h>
#include <l4/thread/thread.h>
#include <l4/con/l4contxtdsi.h>

/* DROPS includes */
#include <l4/env/errno.h>
#include <l4/dsi/dsi.h>

/* OSKit includes */
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "con.h"
#include "vc.h"
#include "dsi_vc.h"
#include "ipc.h"
#include "pslim_func.h"
#include "con_macros.h"
#include "con_config.h"
#include "con-server.h"

extern l4_uint16_t VESA_XRES, VESA_YRES;
extern l4_uint8_t VESA_RES;
extern l4_uint8_t FONT_XRES, FONT_YRES;

/* for txtmode */
extern const char _binary_font_psf_start[];

/******************************************************************************
 * work thread functions                                                      *
 ******************************************************************************/

/******************************************************************************
 * con_dsi_rawcscs_thread                                                     *
 *                                                                            *
 * in:  vc            ... self pointer                                        *
 *                                                                            *
 * for CON_DSI_RAWCSCS type threads                                           *
 ******************************************************************************/
static void
con_dsi_rawcscs_thread(struct l4con_vc *vc)
{
  int ret, i;
  dsi_socket_t *socket;
  dsi_packet_t * packet;
  void *addr_y, *addr_u, *addr_v;
  l4_size_t size;
  l4_uint32_t no;

  /* wait for start notification */
  if ((ret = dsi_thread_worker_wait(&socket)))
    PANIC("wait for start notification failed");

  if ((ret = dsi_thread_worker_started(0)))
    PANIC("resend start notification failed");

  /* work loop */
  while (1)
    {
      /* get packet */
      ret = dsi_packet_get(socket, &packet);
      if (ret && (ret != -DSI_ENOPACKET)) 
	{
	  INFO("get packet failed (%d)\n", ret);
	  continue;
	}
      
      if (ret == -DSI_ENOPACKET) 
	{
	  PANIC("no packet available in blocking mode");
	  continue;
	}
      
      /* get packet number */
      if ((ret = dsi_packet_get_no(socket, packet, &no)))
	INFO("error getting packet no (%d)\n", ret);
      
      /* get data */
      i = 0;
      ret = dsi_packet_get_data(socket, packet, &addr_y, &size);
      if (ret && (ret != -DSI_ENODATA))
	PANIC("error getting Y data areas (%d)", ret);
      ret = dsi_packet_get_data(socket, packet, &addr_u, &size);
      if (ret && (ret != -DSI_ENODATA))
	PANIC("error getting U data areas (%d)", ret);
      ret = dsi_packet_get_data(socket, packet, &addr_v, &size);
      if (ret && (ret != -DSI_ENODATA))
	PANIC("error getting V data areas (%d)", ret);
      
      /* need fb_lock for drawing */
      l4lock_lock(&vc->fb_lock);
      pslim_cscs(vc, 1, (pslim_rect_t *) &vc->dsi_cfg.area,
	         addr_y, addr_u, addr_v, vc->dsi_cfg.type, 1);
      l4lock_unlock(&vc->fb_lock);
      
      /* release packet */
      if ((ret = dsi_packet_commit(socket, packet)))
	INFO("release packet failed (%d)\n", ret);
    }
}

/******************************************************************************
 * con_dsi_rawbmap_thread                                                     *
 *                                                                            *
 * in:  vc            ... self pointer                                        *
 *                                                                            *
 * for CON_DSI_RAWBMAP type threads                                           *
 ******************************************************************************/
static void
con_dsi_bmap_thread(struct l4con_vc *vc)
{
  int ret, i;
  dsi_socket_t *socket;
  dsi_packet_t * p;
  l4_uint32_t num;
  con_pslim_rect_t crect;
  void *addr;
  l4_uint8_t *s;
  struct contxtdsi_coord *str_coord;
  l4_size_t size;
  
  /* wait for start notification */
  if ((ret = dsi_thread_worker_wait(&socket)))
    PANIC("wait for start notification failed");

  if ((ret = dsi_thread_worker_started(0)))
    PANIC("resend start notification failed");

  INFO("CON_DSI_BMAP started id:%d\n", socket->socket_id);
  
  while(1)
    {
      if ((ret = dsi_packet_get(socket, &p)))
	{ 
	  if (ret == -DSI_ENOPACKET)
	    break;
	  else
	    enter_kdebug("rcv get packet failed");
	}
    
      if ((ret = dsi_packet_get_no(socket, p, &num)))
	enter_kdebug("rcv get packet no failed");
    
      /* get packet data */
      if ((ret = dsi_packet_get_data(socket, p, &addr, &size)))
	enter_kdebug("rcv get data failed"); 
      
      if ((ret = dsi_packet_commit(socket, p)))
	enter_kdebug("rcv commit packet failed"); 
    
      str_coord = (struct contxtdsi_coord*) addr;
      s = (l4_uint8_t *) str_coord->str_addr;
      
      if ((vc->mode & CON_OUT)==0)
	return;
      
      crect.w = 8;
      crect.h = (l4_uint16_t) _binary_font_psf_start[3];
      crect.x = str_coord->x * crect.w;
      crect.y = str_coord->y * crect.h;
      
      l4lock_lock(&vc->fb_lock);
      if(vc->fb != 0)
	{
	  for(i = 0; i < str_coord->len; i++) 
	    {
	      pslim_bmap(vc, 1, 
     			 (pslim_rect_t *) &crect, 
			 (pslim_color_t) str_coord->fgc,
			 (pslim_color_t) str_coord->bgc,
			 (void*) &_binary_font_psf_start[crect.h * (*s++) + 4], 
			 pSLIM_BMAP_START_MSB);
	      crect.x += crect.w;
	    }
	}
      l4lock_unlock(&vc->fb_lock);
    }
}

/******************************************************************************
 * con_dsi_vc - IDL server functions                                          *
 ******************************************************************************/
l4_int32_t 
con_dsi_vc_server_smode(sm_request_t *request, 
			l4_uint8_t mode,
			const con_threadid_t *ev_handler,
			sm_exc_t *_ev)
{
  struct l4con_vc *vc;
  vc = (struct l4con_vc *) flick_server_get_local(request);
   
  /* inital state */
  if (vc->mode == CON_OPENING) 
    {
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


l4_int32_t 
con_dsi_vc_server_graph_gmode(sm_request_t *request, 
			      l4_uint8_t *g_mode,
			      l4_uint32_t *xres,
			      l4_uint32_t *yres,
			      l4_uint32_t *bits_per_pixel,
			      l4_uint32_t *bytes_per_pixel,
			      l4_uint32_t *flags,
			      l4_uint32_t *xtxt, l4_uint32_t *ytxt,
			      sm_exc_t *_ev)
{
  *xres = VESA_XRES;
  *yres = VESA_YRES;
  *xtxt = FONT_XRES;
  *ytxt = FONT_YRES;
  return 0;
}

/******************************************************************************
 * con_dsi_vc_server_open                                                     *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      jcp_stream    ... stream description                                  *
 *      cfg           ... stream configuration                                *
 *      ctrl_ds       ... control dataspace                                   *
 *      data_ds       ... data dataspace                                      *
 * out: socket_ref    ... socket reference                                    *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Create socket for standard DSI-VC communication                            *
 ******************************************************************************/
l4_int32_t 
con_dsi_vc_server_open(sm_request_t *request,
		       const con_dsi_jcp_stream_t *jcp_stream,
       		       const con_dsi_cfg_t *cfg,
		       const con_dataspace_t *ctrl_ds,
		       const con_dataspace_t *data_ds,
		       con_dsi_socket_t *socket_ref,
		       sm_exc_t *_ev)
{
  return -CON_ENOTIMPL;
}


/******************************************************************************
 * con_dsi_vc_server_rawset_open                                              *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      jcp_stream    ... stream description                                  *
 *      cfg           ... stream configuration                                *
 *      ctrl_ds       ... control dataspace                                   *
 *      data_ds       ... data dataspace                                      *
 *      area          ... area in virtual framebuffer                         *
 * out: socket_ref    ... socket reference                                    *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Create socket for raw DSI-VC communication (pSLIM SET)                     *
 ******************************************************************************/
l4_int32_t
con_dsi_vc_server_rawset_open(sm_request_t *request, 
			      const con_dsi_jcp_stream_t *jcp_stream,
	       		      const con_dsi_cfg_t *cfg,
			      const con_dataspace_t *ctrl_ds,
			      const con_dataspace_t *data_ds,
			      const con_pslim_rect_t *area,
			      con_dsi_socket_t *socket_ref,
			      sm_exc_t *_ev)
{
  return -CON_ENOTIMPL;
}


/******************************************************************************
 * con_dsi_vc_server_rawcscs_open                                             *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      jcp_stream    ... stream description                                  *
 *      cfg           ... stream configuration                                *
 *      ctrl_ds       ... control dataspace                                   *
 *      data_ds       ... data dataspace                                      *
 *      area          ... area in virtual framebuffer                         *
 *      type          ... type of pixmap's YUV encoding                       *
 * out: socket_ref    ... socket reference                                    *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Create socket for raw DSI-VC communication (pSLIM CSCS)                    *
 ******************************************************************************/
l4_int32_t
con_dsi_vc_server_rawcscs_open(sm_request_t *request,
			       const con_dsi_jcp_stream_t *jcp_stream,
	       		       const con_dsi_cfg_t *cfg,
			       const con_dataspace_t *ctrl_ds,
			       const con_dataspace_t *data_ds,
			       const con_pslim_rect_t *area,
			       l4_uint8_t type,
			       con_dsi_socket_t *socket_ref,
			       sm_exc_t *_ev)
{
  int ret;
  l4thread_t work_tid;
  l4_threadid_t work_l4id, sync_l4id;
  dsi_socket_t * s;
  dsi_jcp_stream_t * jcp_s = (dsi_jcp_stream_t *)jcp_stream;
  dsi_stream_cfg_t * s_cfg = (dsi_stream_cfg_t *)cfg;

  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);

  /* dsi_vc already in use */
  if (vc->mode != CON_OPENING)
    return -CON_EPERM;

  /* open dsi_vc */
  if ((ret = vc_open(vc, CON_OUT)))
    return ret;

  /* start new work thread */
  work_tid = l4thread_create((l4thread_fn_t) con_dsi_rawcscs_thread,
			      (void *) vc, L4THREAD_CREATE_ASYNC);
  work_l4id = l4thread_l4_id(work_tid);

  /* create socket */
  sync_l4id = L4_INVALID_ID;
  ret = dsi_socket_create(*jcp_s, *s_cfg, 
			  (l4dm_dataspace_t *)ctrl_ds, 
		  	  (l4dm_dataspace_t *)data_ds,
	  		  work_l4id, &sync_l4id, 
  			  DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK, &s);
  if (ret) 
    PANIC("socket creation failed (%d)", ret);

  /* get socket reference */
  ret = dsi_socket_get_ref(s,(dsi_socket_ref_t *)socket_ref);
  ASSERT(ret == 0);

  /* set syncronization callback function */
  /* krishna: do we need this? (sync_callback)
   *	ret = dsi_socket_set_sync_callback(s,sync_callback);
   *	ASSERT(ret == 0);
   */

  vc->dsi_mode = CON_DSI_RAWCSCS;
  vc->dsi_cfg.area.x = area->x;
  vc->dsi_cfg.area.y = area->y;
  vc->dsi_cfg.area.w = area->w;
  vc->dsi_cfg.area.h = area->h;
  vc->dsi_cfg.type = type;

  /* done */
  return 0;
}

/******************************************************************************
 * con_dsi_vc_server_bmap_open                                                *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      jcp_stream    ... stream description                                  *
 *      cfg           ... stream configuration                                *
 *      ctrl_ds       ... control dataspace                                   *
 *      data_ds       ... data dataspace                                      *
 *      area          ... area in virtual framebuffer                         *
 *      type          ... type of pixmap's YUV encoding                       *
 * out: socket_ref    ... socket reference                                    *
 *      _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Create socket for raw DSI-VC communication (pSLIM CSCS)                    *
 ******************************************************************************/
l4_int32_t
con_dsi_vc_server_bmap_open(sm_request_t *request,
			    const con_dsi_jcp_stream_t *jcp_stream,
	       		    const con_dsi_cfg_t *cfg,
			    const con_dataspace_t *ctrl_ds,
			    const con_dataspace_t *data_ds,
			    con_dsi_socket_t *socket_ref,
			    sm_exc_t *_ev)
{
  int ret;
  l4thread_t work_tid;
  dsi_socket_t * s;
  l4_threadid_t work_l4id, sync_l4id;
  dsi_jcp_stream_t *jcp_s = (dsi_jcp_stream_t *)jcp_stream;
  dsi_stream_cfg_t *s_cfg = (dsi_stream_cfg_t *)cfg;
  
  struct l4con_vc *vc;
  vc = (struct l4con_vc *) flick_server_get_local(request);

  if ((ret = vc_open(vc, CON_OUT)))
    return ret;
  
  /* start new work thread */
  work_tid = l4thread_create((l4thread_fn_t) con_dsi_bmap_thread, 
			      (void *) vc, L4THREAD_CREATE_SYNC);
  work_l4id = l4thread_l4_id(work_tid);

  /* create socket */
  sync_l4id = L4_INVALID_ID;
  ret = dsi_socket_create(*jcp_s, *s_cfg,
			  (l4dm_dataspace_t *)ctrl_ds,
		  	  (l4dm_dataspace_t *)data_ds,
	  		  work_l4id, &sync_l4id,
  			  DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK
			  , &s);
  if (ret) 
    PANIC("socket creation failed (%d)", ret);

  ret = dsi_socket_get_ref(s,(dsi_socket_ref_t *)socket_ref);
  ASSERT(ret == 0);
  vc->dsi_mode = CON_DSI_BMAP;
  return 0;
}

/******************************************************************************
 * con_dsi_vc_server_connect                                                  *
 *                                                                            *
 * in:  request	      ... Flick request structure                             *
 *      local         ... local socket reference                              *
 *      remote        ... remote socket reference                             *
 * out: _ev           ... Flick exception (unused)                            *
 * ret: 0             ... success                                             *
 *                                                                            *
 * Connect receive socket (local) to send socket (remote).                    *
 ******************************************************************************/
l4_int32_t
con_dsi_vc_server_connect(sm_request_t *request,
			  const con_dsi_socket_t *local,
		       	  const con_dsi_socket_t *remote,
       			  sm_exc_t *_ev)
{
  dsi_socket_t *socket, *r;
  int ret, val;
  struct l4con_vc *vc = (struct l4con_vc *) flick_server_get_local(request);
  
  /* check permission */
  if (!(vc->mode & CON_OUT) || (vc->dsi_mode == CON_NOT_DSI))
    {
      INFO("dsi_vc not ready for connect");
      return -CON_EPERM;
    }
  
  /* socket desc */
  if ((ret = dsi_socket_get_descriptor(local->socket, &socket)))
    {
      INFO("invalid socket id\n");
      return -L4_EINVAL;
    }
  
  /* connection */
  if ((ret = dsi_socket_connect(socket, (dsi_socket_ref_t *)remote)))
    {
      INFO("socket connection failed (%d)\n", ret);
      return ret;
    }

  if ((ret = dsi_thread_start_worker(socket, &val)))
    {
      INFO("socket start worker failed (%d)\n", ret);
      return -L4_EIPC;
    }

  if (val)
    return val;
  
  ret = dsi_socket_get_descriptor(remote->socket, &r);
  
  return 0;
}

/******************************************************************************
 * dsi_vc_loop                                                                *
 *                                                                            *
 * dsi_con_vc - IDL server loop                                               *
 ******************************************************************************/
void
dsi_vc_loop(struct l4con_vc *this_vc)
{
  int ret;
  l4_msgdope_t result;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
	
  flick_init_request(&request, &ipc_buf);
  
  /* pass this_vc reference as implicit arg */
  flick_server_set_local(&request, (void*) this_vc);

  /* tell creator that we are running */
  l4thread_started(NULL);
	
  INFO("vc[%d] running as DSI %x.%02x\n", 
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
	  ret = con_dsi_vc_server(&request);
			
	  if (this_vc->mode == CON_CLOSING) 
	    {
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
	      /* reply and wait for next request */
	      result = flick_server_reply_and_wait(&request);
	      break;
	      
	    default:
	      INFO("Flick dispatch error (%d)!\n", ret);
	      result = flick_server_wait(&request);
	      break;
	    }
	} /* !L4_IPC_IS_ERROR(result) */
		
      /* Ooops, we got an IPC error -> do something */
      PANIC(" Flick IPC error (%#x)", L4_IPC_ERROR(result));
    }
}
