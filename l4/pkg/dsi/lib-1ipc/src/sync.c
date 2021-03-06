/* $Id$ */
/*****************************************************************************/
/**
 * \file	dsi/lib/src/sync.c
 *
 * \brief	Implementation of the synchorinzation thread.
 *
 * \date	07/08/2000
 * \author	Lars Reuther <reuther@os.inf.tu-dresden.de>
 * 
 * This thread handles the synchronization messages from the remote
 * work thread. They are send if the work thread of the remote
 * component waits for a packet to be commited by the local work
 * thread. If the specified packet is not commited yet, the wait flag
 * is set locally in the packet flags and the remote work thread stays
 * in its ipc-call until the local work thread commited the packet an
 * send the wakeup message to us (the synchronization thread).
 *
 * If the synchronization thread receives the wakeup message for a
 * packet (from the local work thread) and the wait flag is not set,
 * the notification pending flag is set in the packet flags and a
 * subsequent synchronization call of the remote work thread returns
 * immediately. This situation can happen if the remote work thread is
 * preemted in the middle of dsi_down.
 *
 * Currently we have two versions of the synchronization thread, one for
 * send components and one for receive components. While the sync thread of 
 * a receive component just handles wait requests for packets to be 
 * acknowledged by the receive component, the sync thread of a send component
 * also implements the mapping/copying of data to the receiver. 
 *
 * \todo Check packet number, especially if provided by the remote component
 *
 * \note It is perfectly legal for the remote component to abort an ongoing
 *      dsi_down() due to the end of transmission. This means, our unblock-
 *      IPC may fail. This is not indicated by the packet_commit() function,
 *      nor can it be found out somehow else.
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/* library includes */
#include <l4/dsi/dsi.h>
#include "__sync.h"
#include "__socket.h"
#include "__thread.h"
#include "__debug.h"
#include "__config.h"

/*****************************************************************************/
/**
 * \brief Synchronization thread, send component.
 * 
 * \param data thread data, pointer to socket descriptor
 */
/*****************************************************************************/ 
void
dsi_sync_thread_send(void * data)
{
  dsi_socket_t * socket;
  dsi_packet_t * packet;
  l4_threadid_t parent = l4thread_l4_id(l4thread_get_parent());
  int ret;
  l4_umword_t dw0,dw1;
  l4_msgdope_t result;
  l4_threadid_t src;

  LOGdL(DEBUG_SYNC_SEND,"up, parent "l4util_idfmt, l4util_idstr(parent));

  /* sanity checks */
  Assert(data != NULL);
  socket = (dsi_socket_t *)data;

  /* wait for parent to send connect message */ 
  ret = l4_ipc_receive(parent,L4_IPC_SHORT_MSG,&dw0,&dw1,
			    L4_IPC_NEVER,&result);
  if (ret || (dw0 != DSI_SYNC_CONNECTED))
    {
      Panic("DSI: sync setup IPC failed (0x%02X)!",ret);
      return;
    }
  Assert(data == (void *)dw1);
  
  LOGdL(DEBUG_SYNC_SEND,"connected.");
  LOGdL(DEBUG_SYNC_SEND,"remote socket %d, work "l4util_idfmt,
        socket->remote_socket.socket,
	l4util_idstr(socket->remote_socket.work_th));

  /* snychronization loop */
  while (1)
    {
      ret = l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,
			     L4_IPC_NEVER,&result);

      if (!ret)
	{
	  LOGdL(DEBUG_SYNC_SEND,"msg from "l4util_idfmt, l4util_idstr(src));
	  LOGdL(DEBUG_SYNC_SEND,"dw0 = %u, dw1 = %u",dw0,dw1);

	  switch (dw0)
	    {
	    case DSI_SYNC_COMMITED:
		LOG_Error("received DSI_SYNC_COMMITED from "l4util_idfmt
		          " (our worker?), wrong DSI version?",
			  l4util_idstr(src));
		continue;
	      if (l4_task_equal(src,socket->work_th))
	        {
	          /*********************************************************
	           * message from local thread
	           ********************************************************/
	          /* packet commited (dw1 -> packet index), wakeup receiver 
	           * work thread */
	          LOGdL(DEBUG_SYNC_SEND,"wakeup receiver, packet %d",dw1);

		  packet = &socket->packets[dw1];

		  /* check if receiver is already waiting */
		  if (packet->flags & DSI_PACKET_RX_WAITING)
		    {
		      LOGdL(DEBUG_SYNC_SEND,"receiver already waiting");
		      LOGdL(DEBUG_SYNC_SEND,"wakeup "l4util_idfmt,
			    l4util_idstr(socket->remote_socket.work_th));

		      if (socket->flags & DSI_SOCKET_MAP ||
			  socket->flags & DSI_SOCKET_COPY){
			  LOG_Error("Mapping/Copying sockets not supported");
		      } else {
			  /* wakeup */
			  ret = l4_ipc_send(socket->remote_socket.work_th,
						 L4_IPC_SHORT_MSG,0,0,
						 L4_IPC_SEND_TIMEOUT_0,
						 &result);
			  if (ret)
			    Error("DSI: IPC error in wakeup 0x%02x",ret);
			}
		      
		      /* reset flags */
		      packet->flags &= (~DSI_PACKET_RX_WAITING);
		    }
		  else
		    {
		      /* receiver not waiting yet */
		      LOGdL(DEBUG_SYNC_SEND,"receiver not yet waiting");

		      /* mark wakeup notification pending */
		      packet->flags |= DSI_PACKET_RX_PENDING;
		      
		    }

		  /* done */
		} else goto e_inv_sender;
	      break;
	    case DSI_SYNC_WAIT:
	      LOG_Error("received DSI_SYNC_WAIT from "l4util_idfmt
		        " (remote worker?), wrong DSI version?",
			l4util_idstr(src));
	      if (l4_task_equal(src,socket->remote_socket.work_th))
		{
		  /**********************************************************
		   * message from remote work thread
		   *********************************************************/
		  /* wait for packet (dw1 -> packet index) */
		  LOGdL(DEBUG_SYNC_SEND,"receiver waits for packet %d",dw1);

		  packet = &socket->packets[dw1];

		  /* check if wakeup notification is already pending */
		  if (packet->flags & DSI_PACKET_RX_PENDING)
		    {
		      LOGdL(DEBUG_SYNC_SEND,"notification pending");
		      LOGdL(DEBUG_SYNC_SEND,"wakeup "l4util_idfmt,
			    l4util_idstr(src));

		      if (socket->flags & DSI_SOCKET_MAP ||
			  socket->flags & DSI_SOCKET_COPY){
			  LOG_Error("Mapping/Copying sockets not supported");
			  continue;
		      } else {
			  /* wakeup */
			  ret = l4_ipc_send(socket->remote_socket.work_th,
						 L4_IPC_SHORT_MSG,0,0,
						 L4_IPC_SEND_TIMEOUT_0,
						 &result);
			  if (ret)
			    Error("DSI: IPC error in reply 0x%02x",ret);
			}

		      /* reset pending flag */
		      packet->flags &= (~DSI_PACKET_RX_PENDING);
		    }
		  else
		    {
		      /* no notification yet */
		      LOGdL(DEBUG_SYNC_SEND,"set wait flag");

		      packet->flags |= DSI_PACKET_RX_WAITING;
		      /* and set the work-thread to that of the sender
		         of this message, as it can be changed. */
		      socket->remote_socket.work_th = src;
		    }
		} else goto e_inv_sender;
	      break;

	    case DSI_SYNC_RELEASE:
	      if (l4_task_equal(src,socket->remote_socket.work_th))
		{
		  /* receive component released packet (dw1 -> packet), call 
		   * notification callback */

		  packet = &socket->packets[dw1];

		  LOGdL(DEBUG_SYNC,"released packet %u (idx %u)",packet->no,dw1);

		  /* call release callback function */
		  if (socket->release_callback)
		    socket->release_callback(socket,packet);

#if RELEASE_DO_CALL
		  /* reply */
		  ret = l4_ipc_send(src,L4_IPC_SHORT_MSG,0,0,
					 L4_IPC_SEND_TIMEOUT_0,
					 &result);
		  if (ret)
		    Error("DSI: sync notification reply failed (0x%02x)!",ret);
#endif		  
		} else goto e_inv_sender;
	      break;

	    default:
		Error("DSI: invalid command (%d) from "l4util_idfmt"! "
		      "Map and copy not handled", dw0, l4util_idstr(src));
	    }
	  continue;

	  e_inv_sender:
	  Error("DSI: ignoring message from "l4util_idfmt, l4util_idstr(src));

	} /* if (!ret) */

      Error("DSI: IPC error in sender sync thread 0x%02x",ret);
    } /* while(1) */

  /* this should never happen */
  Panic("DSI: left sync loop!");
}

/*****************************************************************************/
/**
 * \brief Synchronization thread, receive component
 * \ingroup internal
 * 
 * \param data thread data, pointer to socket descriptor
 */
/*****************************************************************************/ 
void
dsi_sync_thread_receive(void * data)
{
  dsi_socket_t * socket;
  dsi_packet_t * packet;
  l4_threadid_t parent = l4thread_l4_id(l4thread_get_parent());
  int ret;
  l4_umword_t dw0,dw1;
  l4_msgdope_t result;
  l4_threadid_t src;

  LOGdL(DEBUG_SYNC_RECEIVE,"up, parent "l4util_idfmt, l4util_idstr(parent));

  /* sanity checks */
  Assert(data != NULL);
  socket = (dsi_socket_t *)data;

  /* wait for parent to send connect message */ 
  ret = l4_ipc_receive(parent,L4_IPC_SHORT_MSG,&dw0,&dw1,
			    L4_IPC_NEVER,&result);
  if (ret || (dw0 != DSI_SYNC_CONNECTED))
    {
      Panic("DSI: sync setup IPC failed (0x%02X)!",ret);
      return;
    }
  Assert(data == (void *)dw1);
  
  LOGdL(DEBUG_SYNC_RECEIVE,"connected.");
  LOGdL(DEBUG_SYNC_RECEIVE,"remote socket %d, work "l4util_idfmt, 
        socket->remote_socket.socket, 
	l4util_idstr(socket->remote_socket.work_th));

  /* synchronization thread loop */
  while (1)
    {
      ret = l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,
			     L4_IPC_NEVER,&result);      

      if (!ret)
	{
	  LOGdL(DEBUG_SYNC_RECEIVE,"msg from "l4util_idfmt, l4util_idstr(src));
	  LOGdL(DEBUG_SYNC_RECEIVE,"dw0 = %u, dw1 = %u",dw0,dw1);

	  switch (dw0)
	    {
	    case DSI_SYNC_COMMITED:
		LOG_Error("received DSI_SYNC_COMMITED from "l4util_idfmt
		          " (our worker?), wrong DSI version?",
			  l4util_idstr(src));
	      if (l4_task_equal(src,socket->work_th))
		{
		  /*********************************************************
		   * message from local thread
		   ***************************************************************/
		  /* packet commited (dw1 -> packet index), wakeup sender
		   * work thread */
		  LOGdL(DEBUG_SYNC_RECEIVE,"wakeup sender, packet %d",dw1);

		  packet = &socket->packets[dw1];

		  /* check if sender is already waiting */
		  if (packet->flags & DSI_PACKET_TX_WAITING)
		    {
		      LOGdL(DEBUG_SYNC_RECEIVE,"sender already waiting");
		      LOGdL(DEBUG_SYNC_RECEIVE,"wakeup "l4util_idfmt,
			    l4util_idstr(socket->remote_socket.work_th));

		      /* wakeup */
		      ret = l4_ipc_send(socket->remote_socket.work_th,
					     L4_IPC_SHORT_MSG,0,0,
					     L4_IPC_SEND_TIMEOUT_0,
					     &result);
		      if (ret)
			Error("DSI: IPC error in wakeup 0x%02x",ret);

		      /* reset flags */
		      packet->flags &= (~DSI_PACKET_TX_WAITING);
		    }
		  else
		    {
		      /* sender not yet waiting */
		      LOGdL(DEBUG_SYNC_RECEIVE,"sender not yet waiting");

		      /* mark wakeup notification pending */
		      packet->flags |= DSI_PACKET_TX_PENDING;
		    }
		} else goto e_inv_sender;

	      break;
	    case DSI_SYNC_WAIT:
		LOG_Error("received DSI_SYNC_WAIT from "l4util_idfmt
		          " (remote worker?), wrong DSI version?",
			  l4util_idstr(src));
	      if (l4_task_equal(src,socket->remote_socket.work_th))
		{
		  /**********************************************************
		   * message from remote work thread
		   *********************************************************/
		  /* wait for packet (dw1 -> packet index) */
		  LOGdL(DEBUG_SYNC_RECEIVE,"sender waits for packet %d",dw1);

		  packet = &socket->packets[dw1];

		  /* check if wakeup notification is already pending */
		  if (packet->flags & DSI_PACKET_TX_PENDING)
		    {
		      LOGdL(DEBUG_SYNC,"notification pending");
		      LOGdL(DEBUG_SYNC,"wakeup "l4util_idfmt, 
			    l4util_idstr(src));

		      /* receiver already commited packet, reply immediately */
		      ret = l4_ipc_send(src,L4_IPC_SHORT_MSG,0,0,
					     L4_IPC_SEND_TIMEOUT_0,
					     &result);
		      if (ret)
			Error("DSI: IPC error in reply 0x%02x",ret);

		      /* reset pending flag */
		      packet->flags &= (~DSI_PACKET_TX_PENDING);
		    }
		  else
		    {
		      /* no notification yet */
		      LOGdL(DEBUG_SYNC,"set wait flag");

		      /* mark packet */
		      packet->flags |= DSI_PACKET_TX_WAITING;
		      /* and set the work-thread to that of the sender
		         of this message, as it can be changed. */
		      socket->remote_socket.work_th = src;
		    }
		} else goto e_inv_sender;
		  
	      break;

	    default:
		Error("DSI: invalid command (%d) from "l4util_idfmt,
		      dw0, l4util_idstr(src));
	    }
	  continue;

	  e_inv_sender:
	  Error("ignoring message from "l4util_idfmt, l4util_idstr(src));

	} /* if (!ret) */

      Error("DSI: IPC error in receiver sync thread 0x%02x",ret);
    } /* while(1) */

  /* this should never happen */
  Panic("DSI: left sync loop!");
}

/*****************************************************************************/
/**
 * \brief Send connect message to synchronization thread.
 * \ingroup internal
 *
 * \param socket Socket descriptor
 * \return 0 on success (sent start to sync thread), error code otherwise:
 *         - -L4_EINVAL invalid synchronization thread id
 *         - -L4_EIPC   IPC error calling synchronization thread
 */
/*****************************************************************************/ 
int
dsi_start_sync_thread(dsi_socket_t * socket)
{
  int ret;
  l4_msgdope_t result; 

  /* sanity check */
  if (l4_is_invalid_id(socket->sync_th))
    return -L4_EINVAL;

  /* send IPC message */
  ret = l4_ipc_send(socket->sync_th,L4_IPC_SHORT_MSG,DSI_SYNC_CONNECTED,
			 (l4_umword_t)socket,L4_IPC_NEVER,&result);
  if (ret)
    return -L4_EIPC;
  else
    return 0;
}
