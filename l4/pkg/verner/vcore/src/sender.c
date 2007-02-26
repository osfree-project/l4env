/*
 * \brief   Sender for VERNER's core component
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
#include "sender.h"
#include "work_loop.h"

/* configuration */
#include "verner_config.h"


/*****************************************************************************/
/**
 * \brief Get packet and return adress
 * \param control the stream control structure
 * \param packet_index the index of the currently used packet
 * \return 0 on success, otherwise the DSI error code
 *
 */
/*****************************************************************************/
inline int
sender_step (control_struct_t * control, int packet_index)
{
  int ret;
#if BUILD_RT_SUPPORT
  int packetsize;
#endif

  /* do we have already a packet at this index ? */
  if (control->send_packet[packet_index].status != STATE_PACKET_FREE)
  {
    LOGdL (DEBUG_DS, "Warning: We have still one packet! Can't get next one.");
    return 0;
  }

  /* get packet descriptor */
  ret =
    dsi_packet_get (control->send_socket,
		    &control->send_packet[packet_index].packet);
  if (ret)
  {
#if BUILD_RT_SUPPORT
    if (ret != -DSI_ENOPACKET)
#endif
      LOG_Error ("get packet failed (%d)", ret);
    return ret;
  }

#if BUILD_RT_SUPPORT
  /* 
   * in RT-Mode we take the maximum packetsize for calculation of
   * the next free address in dataspace 
   * for non-RT-Mode it's done in sender_commit()
   */

  /* calculate reserved packetsize per packet ! */
  packetsize =
    (control->send_dst_size /
     dsi_socket_get_packet_num (control->send_socket));

  /* 
   * check for wrap-around. If the data area size
   * corresponds to the maximum number of packets and their
   * size, we should not overwrite our own data here 
   */
  control->send_dst_addr += packetsize;
  if ((control->send_dst_addr + packetsize) >
      (control->send_start_addr + control->send_dst_size))
    control->send_dst_addr = control->send_start_addr;
#endif

  /* set sender buffer address */
  control->send_packet[packet_index].addr = control->send_dst_addr;

  /* success  */
  control->send_packet[packet_index].status = STATE_PACKET_GOT;

  /* done */
  return 0;
}



/*****************************************************************************/
/**
 * \brief commit packet
 * \param control the stream control structure
 * \param packet_index the index of the currently used packet
 * \return 0 on success, otherwise DSI error code
 *
 */
/*****************************************************************************/
inline int
sender_commit (control_struct_t * control, int packet_index)
{
  int ret;
#if !BUILD_RT_SUPPORT
  int packetsize;
#endif

  /* do we have a packet ? */
  if (control->send_packet[packet_index].status == STATE_PACKET_FREE)
  {
    LOGdL (DEBUG_DS, "We haven't got a packet yet! Can't commit");
    return 0;
  }

  /* add data */
  ret =
    dsi_packet_add_data (control->send_socket,
			 control->send_packet[packet_index].packet,
			 control->send_dst_addr, PACKETS_IN_BUFFER, 0);
  if (ret)
  {
    LOG_Error ("add data failed (%d)", ret);
    return -1;
  }

  /* set packet number */
  ret =
    dsi_packet_set_no (control->send_socket,
		       control->send_packet[packet_index].packet,
		       control->send_counter++);
  if (ret)
  {
    Panic ("set packet number failed (%d)", ret);
    return -1;
  }

  /* commit packet */
  ret =
    dsi_packet_commit (control->send_socket,
		       control->send_packet[packet_index].packet);
  if (ret)
  {
    LOG_Error ("commit packet failed (%d)", ret);
    return -1;
  }

  /* sucess: we commited this packet - remember this */
  control->send_packet[packet_index].status = STATE_PACKET_FREE;
  control->send_packet[packet_index].addr = NULL;

#if !BUILD_RT_SUPPORT
  /*
   * in non RT-Mode we take the real size for calculation of
   * the next free address in dataspace
   * for RT-Mode it's done in sender_step()
   */
  
  /* calculate reserved packetsize per packet ! */
  packetsize =
    (control->send_dst_size /
     dsi_socket_get_packet_num (control->send_socket));

  /* check if current packetsize > maximum packetsize which should never happend! */
  if (control->plugin_ctrl.packetsize > packetsize)
    Panic ("current packetsize > max. packetsize! This is a bug.");

  /*
   * check for wrap-around. If the data area size
   * corresponds to the maximum number of packets and their
   * size, we should not overwrite our own data here
   */
  control->send_dst_addr += control->plugin_ctrl.packetsize;
  if ((control->send_dst_addr + packetsize) >
      (control->send_start_addr + control->send_dst_size))
    control->send_dst_addr = control->send_start_addr;
#endif
	  
  return 0;
}


/*****************************************************************************/
/**
 * \brief signal sink the end of stream
 *
 */
/*****************************************************************************/
void
sender_signal_eos (control_struct_t * control)
{
  int ret, i;
  /* eos sent ? */
  int packet_index = -1;

#if BUILD_RT_SUPPORT
  /* enable blocking mode for last packets */
  control->send_socket->flags |= DSI_SOCKET_BLOCK;
#endif

  /* check status of all packets */
  for (i = 0; i < MAX_SENDER_PACKETS; i++)
  {
    /* check if it contains to valid data - commit it */
    if (control->send_packet[i].status == STATE_PACKET_FILLED)
    {
      LOGdL (DEBUG_STREAM, "commit valid packet no.%i", i);
      sender_commit (control, i);
    }
    else
      /* if one packet has not been commited and does not contain valid data - use it for eos */
    if (control->send_packet[i].status != STATE_PACKET_FREE)
    {
      if (packet_index != -1)
      {
	Panic
	  ("Two packets mark as non-free. This is a BUG.  Please report.");
      }
      packet_index = i;
    }				/* end if not free */
  }				/* end for all packets */

  /* still a packet got, but not committed yet? */
  if (packet_index == -1)
  {
    packet_index = 0;		/* use first packet */
    LOGdL (DEBUG_STREAM, "get fresh packet for EOS");
    ret =
      dsi_packet_get (control->send_socket,
		      &control->send_packet[packet_index].packet);
    if (ret)
    {
      LOG_Error ("dsi_packet_get() returned %d", ret);
      return;
    }
  }
#if DEBUG_STREAM
  else
    LOGdL (DEBUG_STREAM, "using existing packet (no. %i) for EOS",
	   packet_index);
#endif

  ret =
    dsi_packet_set_no (control->send_socket,
		       control->send_packet[packet_index].packet,
		       control->send_counter++);
  if (ret)
  {
    Panic ("dsi_packet_get() returned %d", ret);
    return;
  }
  /* add eos */
  dsi_packet_add_data (control->send_socket,
		       control->send_packet[packet_index].packet,
		       (l4_addr_t) NULL, 0, DSI_DATA_AREA_EOS);
  if (ret)
  {
    Panic ("dsi_packet_add_data() returned %d", ret);
    return;
  }

  /* commit packet */
  ret =
    dsi_packet_commit (control->send_socket,
		       control->send_packet[packet_index].packet);
  if (ret)
  {
    Panic ("dsi_packet_commit() returned %d\n", ret);
    return;
  }

  /* mark as free */
  control->send_packet[packet_index].status = STATE_PACKET_FREE;


  LOGdL (DEBUG_STREAM, "Stopping stream, sending EOS done.");
  return;

}



/*****************************************************************************/
/**
* \brief Start sending work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* Connects sockets and unlocks a mutex. So we let the work_thread run.
*/
/*****************************************************************************/
int
sender_start (control_struct_t * control, dsi_socket_ref_t * local,
	      dsi_socket_ref_t * remote)
{
  dsi_socket_t *s;
  int ret, packet_index;

  /* ensure we don't create anything */
  work_loop_lock (control);

  /* verbose */
  LOGdL (DEBUG_STREAM, "connected.");


  /* set all packets to free */
  for (packet_index = 0; packet_index < MAX_SENDER_PACKETS; packet_index++)
  {
    control->send_packet[packet_index].status = STATE_PACKET_FREE;
    control->send_packet[packet_index].addr = NULL;
  }

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

  /* get data area */
  ret =
    dsi_socket_get_data_area (control->send_socket, &control->send_start_addr,
			      &control->send_dst_size);
  if (ret)
  {
    Panic ("send_thread(): get data area failed (%d)", ret);
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }

  /* touch */
  l4_touch_rw (control->send_start_addr, control->send_dst_size);

  /* set start addr correct */
  control->send_dst_addr = control->send_start_addr;

  /* beginning w/ first packet */
  control->send_counter = 0;

  /* start work thread */
  work_loop_start_sender (control);

  /* unlock */
  work_loop_unlock (control);

  /* done */
  return 0;
}


/*****************************************************************************/
/**
* \brief Open sender socket
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param socketref socket reference for created DSI-socket
*
* Attach dataspaces to Demuxer-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
int
sender_open_socket (control_struct_t * control,
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

  /* what type of socket? */
#if BUILD_RT_SUPPORT
  flags = DSI_SOCKET_SEND;
#else
  flags = DSI_SOCKET_SEND | DSI_SOCKET_BLOCK;
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
  ret =
    dsi_socket_create (jcp, cfg, (l4dm_dataspace_t *) ctrl_ds,
		       (l4dm_dataspace_t *) data_ds, work_id, &sync_id, flags,
		       &control->send_socket);
  if (ret)
  {
    Panic ("create DSI socket failed");
    /* unlock */
    work_loop_unlock (control);
    return -1;
  }

  /* get socket reference */
  ret = dsi_socket_get_ref (control->send_socket, socketref);
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
* \brief Closes sender socket
* 
* The DSI-Socket is closed after the work_thread is stoped.
*/
/*****************************************************************************/
int
sender_close (control_struct_t * control, l4_int32_t close_socket_flag)
{
  int ret = 0;
  /* ensure we don't create anything */
  work_loop_lock (control);

  /* signal work_thread to stop */
  work_loop_stop (control);

  /* close socket */
  if (close_socket_flag)
  {
    ret = dsi_socket_close (control->send_socket);
    if (ret)
      LOG_Error ("close socket failed (%x)", -ret);
  }
  /* unlock */
  work_loop_unlock (control);

  /* done */
  return ret;
}
