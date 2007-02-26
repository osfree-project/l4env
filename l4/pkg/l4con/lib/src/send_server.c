/* L4/DROPS includes */
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/thread/thread.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/dm_mem/dm_mem.h>

#include <l4/con/l4con.h>

#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/oskit10_l4env/support.h>

/* OSKit includes */
#include <stdio.h>
#include <string.h>

/* local includes */
#include "internal.h"
#include "send_server.h"

void *data_area;
l4_size_t data_size;
dsi_socket_t *send_socket;
extern int scrbuf_size;

/* internal prototypes */
int create_data_ds(l4_size_t, l4dm_dataspace_t *);
int close_data_ds(l4dm_dataspace_t);

/******************************************************************************
 * global structures                                                          *
 ******************************************************************************/

/* global vars */
l4_threadid_t server_l4id = L4_INVALID_ID;

/******************************************************************************
 * DSI send component specific part                                           *
 ******************************************************************************/

/******************************************************************************
 * create_data_ds                                                             *
 *                                                                            *
 * in:  size    ... dataspace size                                            *
 * out: data_ds ... data dataspace id                                         *
 * ret: 0             ... success                                             *
 *      -L4_ENOMEM ... out of memory                                          *
 *      -L4_EINVAL ... invalid dataspace manager name                         *
 *      -L4_EIPC   ... error calling dataspace manager                        *
 *                                                                            *
 * Allocate and attach data dataspace.                                        *
 ******************************************************************************/
int create_data_ds(l4_size_t size, l4dm_dataspace_t *data_ds)
{
  int ret;

  if ((ret = l4dm_mem_open(L4DM_DEFAULT_DSM,size,0,0,"CON send data",data_ds)))
    {
      LOGl("CON: dataspace allocation failed: %s (%d)",
	     l4env_errstr(ret),ret);
      return ret;
    }

  return 0;
}

/******************************************************************************
 * close_data_ds                                                              *
 *                                                                            *
 * in:  data_ds ... data datsapace id                                         *
 * ret:                                                                       *
 *                                                                            *
 * Close data dataspace.                                                      *
 ******************************************************************************/
int close_data_ds(l4dm_dataspace_t data_ds)
{
  l4dm_close(&data_ds);
  return 0;
}

/******************************************************************************
 * public interface of send component                                         *
 ******************************************************************************/

/******************************************************************************
 * send_server_open                                                           *
 *                                                                            *
 * in:  jcp_s      ... stream description                                     *
 *      s_cfg      ... low level stream configuration                         *
 * out: s_ref      ... socket reference                                       *
 *      cds        ... control dataspace                                      *
 *      dds        ... data dataspace                                         *
 * ret:                                                                       *
 *                                                                            *
 * Create new socket.                                                         *
 ******************************************************************************/
l4_int32_t 
send_server_open(dsi_jcp_stream_t *s_jcp,
		 dsi_stream_cfg_t *s_cfg,
		 dsi_socket_ref_t *s_ref, 
		 l4dm_dataspace_t *cds, 
		 l4dm_dataspace_t *dds)
{
  int ret;
  l4_threadid_t  sync_l4id;
  void *addr;  
  l4_size_t size,a;

  ret = create_data_ds((sizeof(struct contxtdsi_coord) * vtc_lines) +
		       scrbuf_size,
		       dds);

  if (ret)
    enter_kdebug("alloc ds failed");
	
  /* create socket */
  *cds = L4DM_INVALID_DATASPACE;
  sync_l4id = L4_INVALID_ID;
  ret = dsi_socket_create(*s_jcp, *s_cfg, cds, dds,
			  server_l4id, &sync_l4id,
			  DSI_SOCKET_SEND | DSI_SOCKET_BLOCK, 
			  &send_socket);
  if (ret) 
    enter_kdebug("create socket failed");

  /* get socket reference */
  ret = dsi_socket_get_ref(send_socket, s_ref);
  if (ret)
    enter_kdebug("ASSERT.0");

  /* make sure the data area is mapped */
  ret = dsi_socket_get_data_area(send_socket,&addr,&size);
  if (ret)
    enter_kdebug("get data area failed");
  
  vtc_scrbuf = (char*) addr + sizeof(struct contxtdsi_coord) * vtc_lines;
  memset(vtc_scrbuf, ' ', scrbuf_size);
  vtc_coord = (struct contxtdsi_coord*) addr;
    
  a = 0;
  while (a < size)
  {
    /* touch data area */
    *((int *)(addr + a)) = 1;
    a += L4_PAGESIZE;
  }

  return 0;
}

/******************************************************************************
 * send_server_close                                                          *
 *                                                                            *
 * ...                                                                        *
 ******************************************************************************/
l4_int32_t 
send_server_close(dsi_component_t *comp)
{
  return 0;
}

/******************************************************************************
 * send_server_connect                                                        *
 *                                                                            *
 * in:  local   ... local component reference                                 *
 *      remote  ... remote socket reference                                   *
 * ret:                                                                       *
 *                                                                            *
 * Connect send socket (local) to receive (remote) socket.                    *
 ******************************************************************************/
int 
send_server_connect(dsi_component_t *comp, dsi_socket_ref_t *remote)
{
  dsi_socket_t *s, *r;
  int ret;
  
  /* get socket descriptor */
  if ((ret = dsi_socket_get_descriptor(comp->socketref.socket, &s)))
    {
      LOGl("invalid socket id");
      return -L4_EINVAL;
    }
  /* connect socket */
  if ((ret = dsi_socket_connect(s, remote)))
    {
      LOGl("connect failed (%d)", ret);
      return ret;
    }
  
  if ((ret = dsi_socket_get_descriptor(remote->socket, &r)))
    {
      LOGl("get socket descriptor failed (%d)", ret);
      return -L4_EINVAL;
    }
  
  return 0;
}

/******************************************************************************
 * send_server_start                                                          *
 *                                                                            *
 * in:  local   ... socket reference                                          *
 * ret: 0          ... success                                                *
 *      -L4_EINVAL ... invalid socket reference                               *
 *      -L4_EIPC   ... start IPC failed                                       *
 *                                                                            *
 * Start work thread.                                                         *
 ******************************************************************************/
int 
send_server_start(dsi_component_t *comp)
{
  int ret;

  /* get socket descriptor */
  if ((ret = dsi_socket_get_descriptor(comp->socketref.socket, &send_socket)))
    {
      LOGl("get socket descriptor failed (%d)", ret);
      return -L4_EINVAL;
    }
   
  if ((ret = dsi_socket_get_data_area(send_socket, &data_area, &data_size)))
    {
      LOGl("get data area failed");
    }

  return ret;
}

/******************************************************************************
 * send_server_stop                                                           *
 *                                                                            *
 * ...                                                                        *
 ******************************************************************************/
int 
send_server_stop(dsi_component_t *comp)
{
  return 0;
}

/******************************************************************************
 * send_server_init                                                           *
 *                                                                            *
 * init send_server                                                           *
 ******************************************************************************/
int 
send_server_init(void)
{
  server_l4id = l4thread_l4_id( l4thread_myself() );
  LOGl("dsitest sender up.");
  return 0;
}

