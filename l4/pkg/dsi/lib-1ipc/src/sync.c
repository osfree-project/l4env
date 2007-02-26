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

#if DEBUG_SYNC_SEND
  INFO("up, parent %x.%x\n",parent.id.task,parent.id.lthread);
#endif

  /* sanity checks */
  Assert(data != NULL);
  socket = (dsi_socket_t *)data;

  /* wait for parent to send connect message */ 
  ret = l4_i386_ipc_receive(parent,L4_IPC_SHORT_MSG,&dw0,&dw1,
			    L4_IPC_NEVER,&result);
  if (ret || (dw0 != DSI_SYNC_CONNECTED))
    {
      Panic("DSI: sync setup IPC failed (0x%02X)!",ret);
      return;
    }
  Assert(data == (void *)dw1);
  
#if DEBUG_SYNC_SEND
  INFO("connected.\n");
  INFO("remote socket %d, work %x.%x\n",socket->remote_socket.socket,
       socket->remote_socket.work_th.id.task,
       socket->remote_socket.work_th.id.lthread);
#endif

  /* snychronization loop */
  while (1)
    {
      ret = l4_i386_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,
			     L4_IPC_NEVER,&result);

      if (!ret)
	{
#if DEBUG_SYNC_SEND
	  INFO("msg from %x.%x\n",src.id.task,src.id.lthread);
	  INFO("dw0 = %u, dw1 = %u\n",dw0,dw1);
#endif
	  switch (dw0)
	    {
	    case DSI_SYNC_COMMITED:
		LOG_Error("received DSI_SYNC_COMMITED from %x.%x (our worker?)"
			  ", wrong DSI version?",
			  src.id.task, src.id.lthread);
		continue;
	      if (l4_task_equal(src,socket->work_th))
	        {
	          /*********************************************************
	           * message from local thread
	           ********************************************************/
	          /* packet commited (dw1 -> packet index), wakeup receiver 
	           * work thread */
#if DEBUG_SYNC_SEND
	          INFO("wakeup receiver, packet %d\n",dw1);
#endif
		  packet = &socket->packets[dw1];

		  /* check if receiver is already waiting */
		  if (packet->flags & DSI_PACKET_RX_WAITING)
		    {
#if DEBUG_SYNC_SEND
		      INFO("receiver already waiting\n");
		      INFO("wakeup %x.%x\n",
			   socket->remote_socket.work_th.id.task,
			   socket->remote_socket.work_th.id.lthread);
#endif
		      if (socket->flags & DSI_SOCKET_MAP ||
			  socket->flags & DSI_SOCKET_COPY){
			  LOG_Error("Mapping/Copying sockets not supported");
		      } else {
			  /* wakeup */
			  ret = l4_i386_ipc_send(socket->remote_socket.work_th,
						 L4_IPC_SHORT_MSG,0,0,
						 L4_IPC_TIMEOUT(0,1,0,0,0,0),
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
#if DEBUG_SYNC_SEND
		      INFO("receiver not yet waiting\n");
#endif
		      /* mark wakeup notification pending */
		      packet->flags |= DSI_PACKET_RX_PENDING;
		      
		    }

		  /* done */
		} else goto e_inv_sender;
	      break;
	    case DSI_SYNC_WAIT:
	      LOG_Error("received DSI_SYNC_WAIT from %x.%x (remote worker?), "
			"wrong DSI version?",
			src.id.task, src.id.lthread);
	      if (l4_task_equal(src,socket->remote_socket.work_th))
		{
		  /**********************************************************
		   * message from remote work thread
		   *********************************************************/
		  /* wait for packet (dw1 -> packet index) */
#if DEBUG_SYNC_SEND
		  INFO("receiver waits for packet %d\n",dw1);
#endif
		  packet = &socket->packets[dw1];

		  /* check if wakeup notification is already pending */
		  if (packet->flags & DSI_PACKET_RX_PENDING)
		    {
#if DEBUG_SYNC_SEND
		      INFO("notification pending\n");
		      INFO("wakeup %x.%x\n",src.id.task,src.id.lthread);
#endif
		      if (socket->flags & DSI_SOCKET_MAP ||
			  socket->flags & DSI_SOCKET_COPY){
			  LOG_Error("Mapping/Copying sockets not supported");
			  continue;
		      } else {
			  /* wakeup */
			  ret = l4_i386_ipc_send(socket->remote_socket.work_th,
						 L4_IPC_SHORT_MSG,0,0,
						 L4_IPC_TIMEOUT(0,1,0,0,0,0),
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
#if DEBUG_SYNC_SEND
		      INFO("set wait flag\n");
#endif
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
#if DEBUG_SYNC
		  INFO("released packet %u (idx %u)\n",packet->no,dw1);
#endif
		  /* call release callback function */
		  if (socket->release_callback)
		    socket->release_callback(socket,packet);

#if RELEASE_DO_CALL
		  /* reply */
		  ret = l4_i386_ipc_send(src,L4_IPC_SHORT_MSG,0,0,
					 L4_IPC_TIMEOUT(0,1,0,0,0,0),
					 &result);
		  if (ret)
		    Error("DSI: sync notification reply failed (0x%02x)!",ret);
#endif		  
		} else goto e_inv_sender;
	      break;

	    default:
		Error("DSI: invalid command (%d) from %x.%x! "
		      "Map and copy not handled"
		      ,dw0, src.id.task, src.id.lthread);
	    }
	  continue;

	  e_inv_sender:
	  Error("DSI: ignoring message from %x.%x",
		src.id.task,src.id.lthread);

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

#if DEBUG_SYNC_RECEIVE
  INFO("up, parent %x.%x\n",parent.id.task,parent.id.lthread);
#endif

  /* sanity checks */
  Assert(data != NULL);
  socket = (dsi_socket_t *)data;

  /* wait for parent to send connect message */ 
  ret = l4_i386_ipc_receive(parent,L4_IPC_SHORT_MSG,&dw0,&dw1,
			    L4_IPC_NEVER,&result);
  if (ret || (dw0 != DSI_SYNC_CONNECTED))
    {
      Panic("DSI: sync setup IPC failed (0x%02X)!",ret);
      return;
    }
  Assert(data == (void *)dw1);
  
#if DEBUG_SYNC_RECEIVE
  INFO("connected.\n");
  INFO("remote socket %d, work %x.%x\n",socket->remote_socket.socket,
       socket->remote_socket.work_th.id.task,
       socket->remote_socket.work_th.id.lthread);
#endif

  /* synchronization thread loop */
  while (1)
    {
      ret = l4_i386_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,
			     L4_IPC_NEVER,&result);      

      if (!ret)
	{
#if DEBUG_SYNC_RECEIVE
	  INFO("msg from %x.%x\n",src.id.task,src.id.lthread);
	  INFO("dw0 = %u, dw1 = %u\n",dw0,dw1);
#endif
	  switch (dw0)
	    {
	    case DSI_SYNC_COMMITED:
		LOG_Error("received DSI_SYNC_COMMITED from %x.%x (our worker?)"
			  ", wrong DSI version?",
			  src.id.task, src.id.lthread);
	      if (l4_task_equal(src,socket->work_th))
		{
		  /*********************************************************
		   * message from local thread
		   ***************************************************************/
		  /* packet commited (dw1 -> packet index), wakeup sender
		   * work thread */
#if DEBUG_SYNC_RECEIVE 
		  INFO("wakeup sender, packet %d\n",dw1);
#endif
		  packet = &socket->packets[dw1];

		  /* check if sender is already waiting */
		  if (packet->flags & DSI_PACKET_TX_WAITING)
		    {
#if DEBUG_SYNC_RECEIVE
		      INFO("sender already waiting\n");
		      INFO("wakeup %x.%x\n",
			   socket->remote_socket.work_th.id.task,
			   socket->remote_socket.work_th.id.lthread);
#endif
		      /* wakeup */
		      ret = l4_i386_ipc_send(socket->remote_socket.work_th,
					     L4_IPC_SHORT_MSG,0,0,
					     L4_IPC_TIMEOUT(0,1,0,0,0,0),
					     &result);
		      if (ret)
			Error("DSI: IPC error in wakeup 0x%02x",ret);

		      /* reset flags */
		      packet->flags &= (~DSI_PACKET_TX_WAITING);
		    }
		  else
		    {
		      /* sender not yet waiting */
#if DEBUG_SYNC_RECEIVE 
		      INFO("sender not yet waiting\n");
#endif
		      /* mark wakeup notification pending */
		      packet->flags |= DSI_PACKET_TX_PENDING;
		    }
		} else goto e_inv_sender;

	      break;
	    case DSI_SYNC_WAIT:
		LOG_Error("received DSI_SYNC_WAIT from %x.%x (remote worker?)"
			  ", wrong DSI version?",
			  src.id.task, src.id.lthread);
	      if (l4_task_equal(src,socket->remote_socket.work_th))
		{
		  /**********************************************************
		   * message from remote work thread
		   *********************************************************/
		  /* wait for packet (dw1 -> packet index) */
#if DEBUG_SYNC_RECEIVE
		  INFO("sender waits for packet %d\n",dw1);
#endif
		  packet = &socket->packets[dw1];

		  /* check if wakeup notification is already pending */
		  if (packet->flags & DSI_PACKET_TX_PENDING)
		    {
#if DEBUG_SYNC
		      INFO("notification pending\n");
		      INFO("wakeup %x.%x\n",src.id.task,src.id.lthread);
#endif
		      /* receiver already commited packet, reply immediately */
		      ret = l4_i386_ipc_send(src,L4_IPC_SHORT_MSG,0,0,
					     L4_IPC_TIMEOUT(0,1,0,0,0,0),
					     &result);
		      if (ret)
			Error("DSI: IPC error in reply 0x%02x",ret);

		      /* reset pending flag */
		      packet->flags &= (~DSI_PACKET_TX_PENDING);
		    }
		  else
		    {
		      /* no notification yet */
#if DEBUG_SYNC
		      INFO("set wait flag\n");
#endif
		      /* mark packet */
		      packet->flags |= DSI_PACKET_TX_WAITING;
		      /* and set the work-thread to that of the sender
		         of this message, as it can be changed. */
		      socket->remote_socket.work_th = src;
		    }
		} else goto e_inv_sender;
		  
	      break;

	    default:
		Error("DSI: invalid command (%d) from %x.%x",
		      dw0, src.id.task, src.id.lthread);
	    }
	  continue;

	  e_inv_sender:
	  Error("ignoring message from %x.%x",
		src.id.task,src.id.lthread);

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
  ret = l4_i386_ipc_send(socket->sync_th,L4_IPC_SHORT_MSG,DSI_SYNC_CONNECTED,
			 (l4_umword_t)socket,L4_IPC_NEVER,&result);
  if (ret)
    return -L4_EIPC;
  else
    return 0;
}
