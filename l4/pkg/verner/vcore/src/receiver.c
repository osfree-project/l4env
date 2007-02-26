/*
 * \brief   Receiver for VERNER's core component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* local */
#include "receiver.h"
#include "work_loop.h"

/* configuration */
#include "verner_config.h"


/*****************************************************************************/
/**
 * \brief signal end of stream via socket event
 * 
 */
/*****************************************************************************/
void
receiver_signal_eos (control_struct_t * control)
{
  int ret;
  
  /* do we have already a packet ? */
  if (control->recv_packet.status > STATE_PACKET_GOT)
  {
    Panic
      ("Arrgghh. Still have a packet, but EOS? That's a bug. Please report.");
  }
  /* no packet should be hold here */
  control->recv_packet.status = STATE_PACKET_FREE;

  /* send event to controller */
  ret = dsi_socket_set_event (control->recv_socket, DSI_EVENT_EOS);
  if (ret)
  {
    Panic ("signal end of stream failed (%d)", ret);
  }
}


/*****************************************************************************/
/**
 * \brief Get packet and return adress
 *
 */
/*****************************************************************************/
inline int
receiver_step (control_struct_t * control)
{
  int ret;
  l4_uint32_t num;
  l4_size_t size;

  /* do we have already a packet ? */
  if (control->recv_packet.status != STATE_PACKET_FREE)
  {
    LOGdL (DEBUG_DS, "Warning: We have still one packet! Can't get next one.");
    return 0;
  }

  /* get packet */
  ret = dsi_packet_get (control->recv_socket, &control->recv_packet.packet);
  if (ret)
  {
#if BUILD_RT_SUPPORT
    if (ret == -DSI_ENOPACKET)
    {
      //LOGdL (DEBUG_STREAM, "dsi_packet_get returned DSI_ENOPACKET");
    }
    else
#endif
      LOG_Error ("get packet failed (%d)", ret);
    return ret;
  }

  /* success ? */
  control->recv_packet.status = STATE_PACKET_GOT;

  /* get packet number */
  ret =
    dsi_packet_get_no (control->recv_socket, control->recv_packet.packet,
		       &num);
  if (ret)
  {
    LOG_Error ("get packet number failed (%d)", ret);
    return ret;
  }

  /* get packet data */
  ret =
    dsi_packet_get_data (control->recv_socket, control->recv_packet.packet,
			 &control->recv_packet.addr, &size);
  if (ret)
  {
    if (ret == -DSI_EEOS)
    {
      LOGdL (DEBUG_STREAM, "dsi_packet_get_data returned DSI_EEOS");
      return ret;
    }
    else
    {
      LOG_Error ("get data failed (%d)", ret);
      return ret;
    }
  }

  /* done */
  return 0;
}


/*****************************************************************************/
/**
 * \brief commit packet
 * 
 */
/*****************************************************************************/
inline int
receiver_commit (control_struct_t * control)
{

  /* do we have a packet ? */
  if (control->recv_packet.status == STATE_PACKET_FREE)
  {
    LOGdL (DEBUG_DS, "We haven't got a packet yet! Can't commit");
    return 0;
  }

  /* immediately acknowledge packet */
  if (dsi_packet_commit (control->recv_socket, control->recv_packet.packet))
  {
    LOG_Error ("commit packet failed.");
    return -1;
  }

  /* sucess: we commited this packet - remember this */
  control->recv_packet.status = STATE_PACKET_FREE;

  return 0;
}

/*****************************************************************************/
/**
* \brief Start receiving work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* Connects sockets and unlocks a mutex. So we let the work_thread run.
*/
/*****************************************************************************/
int
receiver_start (control_struct_t * control, dsi_socket_ref_t * local,
		dsi_socket_ref_t * remote)
{
  dsi_socket_t *s;
  int ret;

  /* ensure we don't create anything */
  work_loop_lock (control);

  /* verbose */
  LOGdL (DEBUG_STREAM, "connected.");

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor (local->socket, &s);
  if (ret)
  {
    Panic ("invalid socket");
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }

  /* connect socket */
  ret = dsi_socket_connect (s, remote);
  if (ret)
  {
    Panic ("connect failed");
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }

  /* no packet should be hold here */
  control->recv_packet.status = STATE_PACKET_FREE;

  /* start work thread */
  work_loop_start_receiver (control);

  /* unlock */
  work_loop_unlock (control);

  /* done */
  return 0;
}


/*****************************************************************************/
/**
* \brief Open receive socket
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param socketref socket reference for created DSI-socket
*
* Attach dataspaces to Sync-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
int
receiver_open_socket (control_struct_t * control,
		      const l4dm_dataspace_t * ctrl_ds,
		      const l4dm_dataspace_t * data_ds,
		      dsi_socket_ref_t * socketref)
{
  int ret;
  l4_threadid_t work_id, sync_id;
  dsi_jcp_stream_t jcp;
  dsi_stream_cfg_t cfg;
  l4_uint32_t flags;

  /* ensure we don't create anything */
  work_loop_lock (control);

  work_id = L4_INVALID_ID;

  /* what type of socket? */
#if BUILD_RT_SUPPORT
  flags = DSI_SOCKET_RECEIVE;
#else
  flags = DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK;
#endif

  /* start work thread */
  ret = work_loop_create (control);
  if (ret < 0)
  {
    Panic ("start work thread failed!");
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }
  work_id = l4thread_l4_id (ret);

  /* create socket */
  cfg.num_packets = PACKETS_IN_BUFFER;
  cfg.max_sg = 2;
  sync_id = L4_INVALID_ID;
  ret = dsi_socket_create (jcp, cfg, (l4dm_dataspace_t *) ctrl_ds,
			   (l4dm_dataspace_t *) data_ds, work_id, &sync_id,
			   flags, &control->recv_socket);
  if (ret)
  {
    Panic ("create DSI socket failed");
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }

  /* get socket reference */
  ret = dsi_socket_get_ref (control->recv_socket, socketref);
  if (ret)
  {
    Panic ("get socket ref failed");
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }

  /* unlock */
  work_loop_unlock (control);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
* \brief Closes receive socket
* 
* The DSI-Socket is closed after the work_thread is stoped.
*/
/*****************************************************************************/
int
receiver_close (control_struct_t * control, l4_int32_t close_socket_flag)
{
  int ret = 0;

  /* ensure we don't create anything */
  work_loop_lock (control);

  /* signal work_thread to stop */
  work_loop_stop (control);

  /* close socket */
  if (close_socket_flag)
  {
    ret = dsi_socket_close (control->recv_socket);
    if (ret)
      LOG_Error ("close socket failed (%x)", -ret);
  }
  /* unlock */
  work_loop_unlock (control);
  /* done */
  return ret;
}
