/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/src/packet.c
 * \brief  DROPS Stream Interface. DSI packet handling.
 *
 * \date   07/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * \note   To avoid the automatic packet numbering, set
 *	   DO_PACKET_NUMBERING  to 0. The packet number can be set
 *	   using the dsi_packet_set_no function.
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/* library includes */
#include <l4/dsi/dsi.h>
#include "__packet.h"
#include "__sync.h"
#include "__socket.h"
#include "__debug.h"
#include "__config.h"

/* use automatic packet numbering */
#define DO_PACKET_NUMBERING  0

/*****************************************************************************
 * helpers
 *****************************************************************************/

static void do_socket_longjmp(dsi_socket_t *socket){
    _longjmp(socket->packet_get_abort_env, 1);
}

/*****************************************************************************/
/**
 * \brief Check if packet points to a valid packet descriptor of socket.
 * \ingroup internal
 * 
 * \param socket         Socket descriptor
 * \param packet         Packet descriptor
 *
 * \return 1 if \a packet is a valid packet descriptor, 0 otherwise 
 */
/*****************************************************************************/ 
static inline int
__is_valid_packet(dsi_socket_t * socket, dsi_packet_t * packet)
{
  return ((packet >= socket->packets) && 
	  ((l4_addr_t)packet < 
	   ((l4_addr_t)socket->packets + 
	    socket->header->num_packets * sizeof(dsi_packet_t))));
}


#define set_socket_flag(s, f)		\
    __asm__ __volatile__ (		\
	"orl	%1,%0	\n\t"		\
	:				\
	:"m" ((s)->flags), "g" (f)	\
	:"memory"			\
	)

#define reset_socket_flag(s, f)		\
    __asm__ __volatile__ (		\
	"andl	%1,%0	\n\t"		\
	:				\
	:"m" ((s)->flags), "g" (~f)	\
	:"memory"			\
	)


/*****************************************************************************/
/**
 * \brief Calculate packet index.
 * \ingroup internal
 * 
 * \param socket         Socket descriptor
 * \param packet         Packet descriptor
 *
 * \return index of packet in packet array
 *
 * \note No sanity checks!
 */
/*****************************************************************************/ 
static inline int
__get_packet_index(dsi_socket_t * socket, dsi_packet_t * packet)
{
  return (int)(((l4_addr_t)packet - (l4_addr_t)socket->packets) / 
	       sizeof(dsi_packet_t));
}

/*****************************************************************************/
/**
 * \brief Send release packet notification to send component.
 * \ingroup internal
 * 
 * \param socket         Socket descriptor
 * \param packet         Packet descriptor
 *
 * \return 0 on success (notification sent), error code otherwise:
 *         - -L4_EIPC IPC error calling send components sync thread
 */
/*****************************************************************************/ 
static inline int
__send_release_notification(dsi_socket_t * socket, dsi_packet_t * packet)
{
  int ret;
  l4_msgdope_t result;
  l4_uint32_t p;
#if RELEASE_DO_CALL
  l4_umword_t dummy;
#endif

  LOGdL(DEBUG_RECEIVE_PACKET,"packet %d",packet->no);
  LOGdL(DEBUG_RECEIVE_PACKET,"remote sync %x.%x",
        socket->remote_socket.sync_th.id.task,
        socket->remote_socket.sync_th.id.lthread);
  
  /* get packet index */
  p = __get_packet_index(socket,packet);

  /* send notification message */
  do
    {
      /* call send component */
#if RELEASE_DO_CALL
      ret = l4_ipc_call(socket->remote_socket.sync_th,L4_IPC_SHORT_MSG,
			     DSI_SYNC_RELEASE,p,L4_IPC_SHORT_MSG,&dummy,
			     &dummy,L4_IPC_NEVER,&result);
#else
      ret = l4_ipc_send(socket->remote_socket.sync_th,L4_IPC_SHORT_MSG,
			     DSI_SYNC_RELEASE,p,L4_IPC_NEVER,&result);
#endif

      if (ret == L4_IPC_SETIMEOUT)
	{
#if 1
	  LOG_Error("DSI: send timeout calling sender, ignored.");
#endif
	  l4thread_sleep(1);
	}
    }
  while (ret == L4_IPC_SETIMEOUT);

  if (ret)
    {
      Error("DSI: error sending release notification (0x%02x)!",ret);
      return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Find unused scatter gather list element.
 * \ingroup internal
 * 
 * \param  socket        Socket descriptor.
 * \retval sg_elem       Index of empty scattet gather list element
 *
 * \retval 0		got unused element, \c sg_elem contains index to
 *			empty scatter-gather list element
 * \retval -DSI_ENOSGELEM no scatter-gather list element found
 *
 * Unused packets have data address set to 0xFFFFFFFF.
 */
/*****************************************************************************/ 
static inline int
__get_sg_elem(dsi_socket_t * socket, int * sg_elem)
{
  int i = socket->next_sg_elem;

  *sg_elem = -1;

  /* search packet */
  do
    {
      if (cmpxchg32(&socket->sg_lists[i].flags,
		    DSI_SG_ELEM_UNUSED,DSI_SG_ELEM_USED))
	{
	  /* found */
	  socket->next_sg_elem = (i + 1) % socket->header->num_sg_elems;
	  break;
	}

      i = (i + 1) % socket->header->num_sg_elems;
    }
  while (i != socket->next_sg_elem);

  if (i == socket->next_sg_elem)
    {
      /* this should never happen */
      Panic("DSI: no scatter gather list element available!");
      return -DSI_ENOSGELEM;
    }
  else
    {
      *sg_elem = i;
      return 0;
    }
}

/*****************************************************************************/
/**
 * \brief Try to lock next send packet in packet ring list.
 * \ingroup internal
 * 
 * \param  socket        Socket descriptor
 * \retval packet        Index of next send packet
 *
 * \retval 0			on success (\a packet contains valid index)
 * \retval -DSI_ENOPACKET	- non-blocking mode: next packet still used
 *				  by the receive component
 *				- blocking mode: block/unblock-ipc returned
 *				  an error
 * \retval -DSI_ECONNECT	blocking mode: communication peer does
 *				not exist
 *
 * If the packet is still used by the receiver, either block and wait until
 * it is released (if DSI_SOCKET_BLOCK flag is set in socket descriptor) or
 * return an error otherwise.
 */
/*****************************************************************************/ 
static inline int
__get_send_packet(dsi_socket_t * socket, int * packet)
{
  dsi_sync_msg_t msg;
  dsi_packet_t * p = &socket->packets[socket->next_packet];
  int err;

  LOGdL(DEBUG_SEND_PACKET,"trying to get packet %d",socket->next_packet);

  /* Abort-Flag set in between? */
  if(socket->flags & DSI_SOCKET_BLOCK_ABORT) goto e_eeos;
  if (!dsi_trylock(&p->tx_sem)){
      /* We did not get the packet. This is not the normal case. */
      if (SOCKET_BLOCK(socket)) {
	  /* Blocking socket. Prepare for waiting. Our peer will not
	     see that we block until we are inside dsi_down(). This
	     means, the preparation might be for nothing, and that our
	     threads must not count on our blocking only because it
	     sees the sync_callback or the DSI_SOCKET_BLOCKING_IN_GET
	     flag set. */

	  if(_setjmp(socket->packet_get_abort_env)) goto e_eeos;

	  /* Tell our threads we are blocking */
	  set_socket_flag(socket, DSI_SOCKET_BLOCKING_IN_GET);

	  /* After(!) setting the blocking flag, check again if we should
	     abort. */
	  if(socket->flags & DSI_SOCKET_BLOCK_ABORT) goto e_eeos;

	  if (SOCKET_SYNC_CALLBACK(socket)){
	      /* packet not available, call synchronization callback */
	      socket->sync_callback(socket,socket->packet_count,
				    DSI_SYNC_NO_SEND_PACKET);
	  }

	  /* block now on someone */
	  // msg.work_th = socket->remote_socket.sync_th;
	  msg.work_th = L4_INVALID_ID;
	  msg.packet = socket->next_packet;
	  msg.rcv = L4_IPC_SHORT_MSG;
	  err = dsi_down(&p->tx_sem,msg);

	  /* Not waiting anymore */
	  reset_socket_flag(socket, DSI_SOCKET_BLOCKING_IN_GET);

	  if(err!=-1 && err){
	  	return ((err & L4_IPC_ERROR_MASK) == L4_IPC_ENOT_EXISTENT)?
	  	        -DSI_ECONNECT:-DSI_ENOPACKET;
	  }
      }
      else {
	  /* Most unusual case: nonblocking socket which does not get a
	     packet. */
	  return -DSI_ENOPACKET;
      }
  }

  /* got the packet */
  *packet = socket->next_packet;
  socket->next_packet = 
    (socket->next_packet + 1) % socket->header->num_packets;

  /* setup packet descriptor */
#if DO_PACKET_NUMBERING
  p->no = socket->packet_count++;
#endif
  p->flags &= (~DSI_PACKETS_USER_MASK);
  p->sg_len = 0;
  p->sg_list = DSI_SG_ELEM_LAST;

  return 0;

e_eeos:
  reset_socket_flag(socket,
		    DSI_SOCKET_BLOCKING_IN_GET |
		    DSI_SOCKET_BLOCK_ABORT);
  return -DSI_EEOS;
}


/*****************************************************************************/
/**
 * \brief Try to lock next receive packet in packet ring list.
 * \ingroup internal
 * 
 * \param  socket        Socket descriptor
 * \retval packet        Index of next receive packet
 *
 * \retval 0			on success (\a packet contains valid index)
 * \retval -DSI_ENOPACKET	- non-blocking mode: next packet still used
 *				  by the send component
 *				- blocking mode: block/unblock-ipc returned
 *				  an error
 * \retval -DSI_EEOS		aborted by dsi_packet_get_abort()
 * \retval -DSI_ECONNECT	blocking mode: communication peer does
 *				not exist
 *
 * If the packet is not available yet (sender didn't commit data), either
 * block and wait until sender commited data (if DSI_SOCKET_BLOCK flag is
 * set in socket descriptor) or return an error otherwise.
 *
 * There are two ways to map/copy the data of a packet (required if 
 * DSI_SOCKET_MAP or DSI_SOCKET_COPY flags are set for the socket). If 
 * \a dsi_down calls the sender sync thread to wait for the next packet,
 * the sender maps/copies the data in the reply message 
 * (see dsi_sync_thread_send()). If we get the packet without calling 
 * the remote sync thread, we must map/copy the data explicitly.
 */
/*****************************************************************************/ 
static inline int 
__get_receive_packet(dsi_socket_t * socket, int * packet)
{
  dsi_sync_msg_t msg;
  int result = -1;

  LOGdL(DEBUG_RECEIVE_PACKET,"trying to get packet %d",socket->next_packet);

  /* Abort-Flag set in between? */
  if(socket->flags & DSI_SOCKET_BLOCK_ABORT) goto e_eeos;
  if(!dsi_trylock(&socket->packets[socket->next_packet].rx_sem)){
      if (SOCKET_BLOCK(socket)) {
	  /* Blocking socket. Prepare for waiting */

	  /* Setup synchronization message */
	  // msg.sync_th = socket->remote_socket.sync_th; 
	  msg.work_th = L4_INVALID_ID;
	  msg.packet = socket->next_packet;
	  if (socket->flags & DSI_SOCKET_MAP) {
	      LOG_Error("Mapping sockets not supported");
	      return -L4_EINVAL;
 	  } else if (socket->flags & DSI_SOCKET_COPY) {
	      LOG_Error("Copying sockets not supported");
	      return -L4_EINVAL;
	  } else
	      msg.rcv = L4_IPC_SHORT_MSG;

	  if(_setjmp(socket->packet_get_abort_env)) goto e_eeos;

	  /* Tell our threads we are blocking */
	  set_socket_flag(socket, DSI_SOCKET_BLOCKING_IN_GET);

	  /* After(!) setting the blocking flag, check again if we should
	     abort. */
	  if(socket->flags & DSI_SOCKET_BLOCK_ABORT) goto e_eeos;

	  if (SOCKET_SYNC_CALLBACK(socket)) {
	      socket->sync_callback(socket,
				    socket->packets[socket->next_packet].no,
				    DSI_SYNC_NO_RECEIVE_PACKET);
	  }

	  /* block */
	  result = dsi_down(&socket->packets[socket->next_packet].rx_sem,msg);

	  /* Not waiting anymore */
	  reset_socket_flag(socket, DSI_SOCKET_BLOCKING_IN_GET);

	  if(result!=-1 && result)
	  	return ((result & L4_IPC_ERROR_MASK)==L4_IPC_ENOT_EXISTENT)?
	  		-DSI_ECONNECT:-DSI_ENOPACKET;

      }	else {
	  /* Most unusual case: nonblocking socket which does not get a
	     packet. */
	  return -DSI_ENOPACKET;
      }
  } /* got the packet */
      
  LOGdL(DEBUG_RECEIVE_PACKET,"got packet %d",socket->next_packet);

  /* got the packet */
  *packet = socket->next_packet;

  socket->next_packet = 
    (socket->next_packet + 1) % socket->header->num_packets;

  /* done */
  return 0;

  e_eeos:
  reset_socket_flag(socket,
		    DSI_SOCKET_BLOCKING_IN_GET |
		    DSI_SOCKET_BLOCK_ABORT);
  return -DSI_EEOS;
}

/*****************************************************************************/
/**
 * \brief Commit send packet.
 * \ingroup internal
 * 
 * \param  socket        Socket descriptor
 * \param  packet        Packet descriptor
 *
 * \return 0 on success, error code otherwise
 *         - -L4_EINVAL     invalid packet descriptor
 *         - -DSI_ENODATA   tried to commit empty packet
 *	   - -DSI_ENOPACKET peer in blocking mode: committing required a
 *			    sync-message which failed
 *
 * The receiver can now use this packet. If the receiver is already waiting
 * for this packet, send wakup message to our synchronization thread.
 */
/*****************************************************************************/ 
static inline int
__commit_send_packet(dsi_socket_t * socket, dsi_packet_t * packet)
{ 
  dsi_sync_msg_t msg;

#if DO_SANITY
  /* check packet descriptor */
  if (!__is_valid_packet(socket,packet))
    return -L4_EINVAL;
#endif

  if (packet->sg_len == 0)
    return -DSI_ENODATA;

  /* set packet flags */
  if (SOCKET_RELEASE_CALLBACK(socket))
    packet->flags |= DSI_PACKET_RELEASE_CALLBACK;

  /* we possibly need to wakup receiver, setup synchronization message */
  /* the receiver is waiting for us */
  msg.work_th = socket->remote_socket.work_th; 
  msg.packet = __get_packet_index(socket,packet);

  LOGdL(DEBUG_SEND_PACKET,"commiting packet %d",msg.packet);

  /* commit packet, the rx_sem counter of the packet is used to synchronize 
   * valid send data (see __get_receive_packet) */
  if(dsi_up(&packet->rx_sem,msg)) return -DSI_ENOPACKET;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Commit (release) received packet.
 * \ingroup internal
 *
 * \param socket		socket descriptor
 * \param packet		packet to commit
 *
 * \retval 0			success
 * \retval -L4_EINVAL	        invalid packet descriptor
 * \retval -L4_EIPC		IPC error sending release notifcation to
 *				sender
 * \retval -DSI_ENOPACKET	peer in blocking mode: IPC error sending
 *				unblock-notification
 *
 * The send component can now use this packet for the next send packet. If
 * the sender is already waiting for the packet, send wakeup message.
 */
/*****************************************************************************/ 
static inline int
__commit_receive_packet(dsi_socket_t * socket, dsi_packet_t * packet)
{
  dsi_sync_msg_t msg;
  int i,j,sg_elem;
  int ret = 0;

#if DO_SANITY
  /* check packet descriptor */
  if (!__is_valid_packet(socket,packet))
    return -L4_EINVAL;
#endif

  /* reset get index so that the sender can use dsi_packet_get_data to read 
   * the data areas */
  packet->sg_idx = packet->sg_list;

  /* check if we should send release notification to send component */
  if (PACKET_RELEASE_CALLBACK(packet))
    /* send release notification */
    ret = __send_release_notification(socket,packet);

  /* we possibly need to wakup the sender, setup synchronization message */
  /* the sender is waiting for us. */
  msg.work_th = socket->remote_socket.work_th; 
  msg.packet = __get_packet_index(socket,packet);

  /* release scatter gather list elements */
  sg_elem = packet->sg_list;
  for (i = 0; i < packet->sg_len; i++)
    {
      if (sg_elem == DSI_SG_ELEM_LAST)
	{
	  Panic("DSI: corrupted sgatter gather list");
	  return -L4_EINVAL;
	}

      j = sg_elem;
      sg_elem = socket->sg_lists[j].next;
      socket->sg_lists[j].flags = DSI_SG_ELEM_UNUSED;
    }

  if (sg_elem != DSI_SG_ELEM_LAST)
    {
      Panic("DSI: corrupted sgatter gather list");
      return -L4_EINVAL;
    }

  /* release packet, the tx_sem counter of the packet is used to synchronize 
   * the free packets (see __get_send_packet) */
  if(dsi_up(&packet->tx_sem,msg)) return -DSI_ENOPACKET;

  /* done */
  return ret;
}

/*****************************************************************************
 * API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Request next send/receive packet.
 * \ingroup packet
 * 
 * \param  socket        Socket descriptor
 * \retval packet        Pointer to next packet
 * 
 * \retval 0			on success (\a packet contains valid index)
 * \retval -DSI_ENOPACKET	non-blocking mode: next packet still used
 *				by the receive component
 * \retval -DSI_ENOPACKET	blocking mode: block/unblock-ipc returned
 *				an error
 * \retval -DSI_EEOS            aborted by dsi_packet_get_abort()
 * \retval -DSI_ECONNECT        blocking mode: communication peer does
 *                              not exist
 */
/*****************************************************************************/ 
int 
dsi_packet_get(dsi_socket_t * socket, dsi_packet_t ** packet)
{
  int ret,i;

#if 0
  LOGL("socket at 0x%08x",(unsigned)socket);
#endif

#if DO_SANITY
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;
#endif

  /* get next send/receive packet */
  if (IS_SEND_SOCKET(socket))
    ret = __get_send_packet(socket,&i);
  else if (IS_RECEIVE_SOCKET(socket))
    ret = __get_receive_packet(socket,&i);
  else
    {
      Panic("DSI: invalid socket");
      return -L4_EINVAL;
    }

#if 0
  LOGL("ret = %d",ret);
#endif

  if (ret && (ret != -DSI_ENOPACKET) && ret!=-DSI_EEOS)
    {
      Error("DSI: get packet failed: %s (%d)",l4env_errstr(ret),ret);
      return ret;
    }

  if (ret) return ret;

#if 0
#if (DEBUG_SEND_PACKET || DEBUG_RECEIVE_PACKET)
  LOGL("got packet %d",i);
#endif
#endif

  /* setup packet descriptor */
  *packet = &socket->packets[i];
  if (IS_SEND_SOCKET(socket))
    (*packet)->sg_idx = DSI_SG_ELEM_LAST;
  else
    (*packet)->sg_idx = (*packet)->sg_list;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Abort an ongoing packet_get() in the work-thread.
 *
 * \ingroup packet
 * 
 * \param  socket        Socket descriptor
 * 
 * \retval 0 on success		(\a packet_get was ongoing), error otherwise
 * \retval -DSI_ENOPACKET	the work-thread was not blocking inside
 *				a packet_get. The next dsi_packet_get() will
 *				return -DSI_EEOS
 * \retval -DSI_EINVAL		invalid socket descriptor
 *
 * This function can be used to abort a dsi_packet_get(), which was
 * issued by the work-thread of the socket. After the abort, the
 * socket is in an undefined state regarding the packet list. This
 * means, further calls to dsi_packet_get() deliver undefined results.
 * The same hold for the communication peer.
 *
 * The intended use of this function is to unblock the worker if the socket
 * should be shut down by an service thread.
 *
 * \note This function must not be called if the dsi_packet_get() was not
 *	 issued by the work-thread of the socket.
 *
 * \note This function must not called more than once per socket.
 *
 */
/****************************************************************************/ 
int 
dsi_packet_get_abort(dsi_socket_t * socket)
{
#if DO_SANITY
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;
#endif

  set_socket_flag(socket, DSI_SOCKET_BLOCK_ABORT);

  if(socket->flags & DSI_SOCKET_BLOCKING_IN_GET){
      /* it is actually inside a packet get. The longjmp-environment is
	 therefore valid. ex-regs the thread to the longjump-function.
      */
      l4_threadid_t preempter = L4_INVALID_ID, pager=L4_INVALID_ID;
      l4_umword_t	*stack = socket->abort_stack+sizeof(socket->abort_stack);
      l4_umword_t	dummy;

      *--stack=(l4_umword_t)socket;	// the argument
      *--stack=0;	// faked return address
      
      l4_thread_ex_regs(socket->work_th, (l4_umword_t)do_socket_longjmp,
			(l4_umword_t)stack, &preempter, &pager,
			&dummy, &dummy, &dummy);
      return 0;
  }

  return -DSI_ENOPACKET;
}


/*****************************************************************************/
/**
 * \brief Wait for specific packet.
 * \ingroup packet
 *
 * \param  socket        Socket descriptor
 * \param  packet        Packet descriptor
 * 
 * \retval 0			success (\a packet contains valid packet)
 * \retval -DSI_ENOPACKET	- non-blocking mode: next packet still used
 *				  by the receive component
 *				- blocking mode: block/unblock-ipc returned
 *				  an error
 * \retval -DSI_ENODATA		tried to commit empty send packet
 *
 * \Todo   Implemented dsi_packet_get_nr().
 *
 * This function requests a packet with a given sequence number. It should be
 * used together with unblocking synchronisation.
 *
 * \note   The range of sequence numbers is limited, resulting in a wraparound
 *	   sometimes. When calculating the packet number directly using
 *	   the remainder of a division, we end up in having steps in our
 *	   sequence.
 *	   Thus we have a window of valid framenumbers represented by 
 *	   a number limited in size, out of an inifinite range of framenumbers.
 *	   This also requires an additional offset that is added prior
 *	   to packet-place calculation out of the packet nr. But, we will deal
 *	   with this later. For now, it is sufficient to have 32bit-IDs, this
 *	   lasts for about 50days with 1kHz framerate.
 */
/*****************************************************************************/ 
int 
dsi_packet_get_nr(dsi_socket_t * socket, unsigned nr, dsi_packet_t ** packet)
{
  /* not implemented yet */
  return 0;
}

/*****************************************************************************/ 
/**
 * \brief Commit send / release receive packet.
 * \ingroup packet
 *
 * \param socket	socket descriptor
 * \param packet	packet to commit
 *
 * \retval 0		  success
 * \retval -L4_EINVAL  invalid socket/packet descriptor
 * \retval -DSI_ENODATA   tried to commit empty send packet
 * \retval -DSI_ENOPACKET peer in blocking mode: committing required a
 *			  sync-message which failed
 */
/*****************************************************************************/ 
int
dsi_packet_commit(dsi_socket_t * socket, dsi_packet_t * packet)
{
  int ret;

#if DO_SANITY
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;
#endif

  /* commit send/recveive packet */
  if (IS_SEND_SOCKET(socket))
    ret = __commit_send_packet(socket,packet);
  else if (IS_RECEIVE_SOCKET(socket))
    ret = __commit_receive_packet(socket,packet);
  else
    {
      Error("DSI: invalid socket");
      return -L4_EINVAL;
    }
  
  if (ret)
    {
      Error("DSI: commit packet failed: %s (%d)",l4env_errstr(ret),ret);
      return ret;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Add data area to packet's scatter gather list.
 * \ingroup packet
 * 
 * \param socket         Socket descriptor
 * \param packet         Packet descriptor
 * \param addr	         data start address
 * \param size	         data size
 * \param flags	         data area flags, can be a combination of
 *			 -# DSI_DATA_AREA_GAP to add a gap, address ignored
 *			 -# DSI_DATA_AREA_EOS to signal the end of stream,
 *			    address and size are ignored
 *			 -# DSI_DATA_AREA_PHYS to add a packet whose physical
 *			    address is given in addr
 * 
 * \return 0 on success, error code otherwise:
 *         - -L4_EINVAL  invalid socket/packet/data area
 *         - -DSI_ESGLIST   scatter gather list too long
 *         - -DSI_ENOSGELEM no scatter gather element available
 */
/*****************************************************************************/ 
int 
dsi_packet_add_data(dsi_socket_t * socket, dsi_packet_t * packet, 
		    void * addr, l4_size_t size,
		    l4_uint32_t flags)
{
  l4_addr_t offs;
  int ret,sg_elem;

#if DO_SANITY
  /* check socket */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* check packet */
  if (!__is_valid_packet(socket,packet))
    return -L4_EINVAL;

  /* only send components can add data */
  if (!IS_SEND_SOCKET(socket))
    return -L4_EINVAL;
#endif

  /* check scatter gather list */
  if (packet->sg_len >= socket->header->max_sg_len)
    {
      /* exceeded max scatter gather list length */
      Error("DSI: scatter gather list too long (%d)",packet->sg_len + 1);
      return -DSI_ESGLIST;
    }

  /* calculate address offset, the scatter gather list elements stor the 
   * offset relative to the start of the data area */
  if (!(flags & DSI_DATA_AREA_GAP) && 
      !(flags & DSI_DATA_AREA_EOS) &&
      !(flags & DSI_DATA_AREA_PHYS))
    {
      /* data area contains valid data, really check offset/size */ 
      offs = addr - socket->data_area;
      if ((offs > socket->data_size) || 
	  ((offs + size) > socket->data_size) ||
	  (size == 0))
	{
	  /* invalid data area */
	  Error ("DSI: invalid data area (addr 0x%08x, size %u)",
		 (l4_addr_t)addr,size);
	  return -L4_EINVAL;
	}
    }
  else if(flags & DSI_DATA_AREA_PHYS) {
    offs = (l4_addr_t)addr;
  } else
    /* data area contains no data */
    offs = 0;

  /* add scatter gather list element */
  ret = __get_sg_elem(socket,&sg_elem);
  if (ret)
    return -DSI_ENOSGELEM;

  socket->sg_lists[sg_elem].addr = offs;
  socket->sg_lists[sg_elem].size = size;
  socket->sg_lists[sg_elem].flags |= (flags & DSI_SG_ELEM_USER_MASK);
  socket->sg_lists[sg_elem].next = DSI_SG_ELEM_LAST;

  if (packet->sg_len == 0)
    {
      /* add first element in scatter gather list */
      packet->sg_list = sg_elem;
      packet->sg_idx = sg_elem;
      packet->sg_len = 1;
    }
  else
    {
      /* add element at the end of the list */
      socket->sg_lists[packet->sg_idx].next = sg_elem;
      packet->sg_idx = sg_elem;
      packet->sg_len++;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Get next data area from packet.
 * \ingroup packet
 * 
 * \param  socket        Socket descriptor
 * \param  packet        Packet descriptor
 * \retval addr	         Start address (absolute) of data area
 * \retval size          Size of data area
 * 
 * \return 0 on success (\a addr and \a size describe a valid data area), 
 *         error code otherwise:
 *	   - -DSI_EPHYS    chunk describes a piece of physical memory.
 *			   addr contains the physical address of the data.
 *         - -DSI_EGAP     chunk describes gap in stream, size is the size
 *                         of the gap
 *	   - -DSI_EEOS	   packet signals the end of the stream
 *         - -DSI_ENODATA  packet contains no more data
 *         - -L4_EINVAL    invalid socket or packet descriptor
 * 
 * Get next data area from packet. If the packet contains more than one
 * area, get_data returns the next area in the scatter gather list and
 * dsi_packet_get_data must be called repeatedly to get the rest of the
 * scatter gather list.
 */
/*****************************************************************************/ 
int 
dsi_packet_get_data(dsi_socket_t * socket, dsi_packet_t * packet,
		    void **addr, l4_size_t * size)
{
  l4_uint32_t nr;

#if DO_SANITY
  /* check socket */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* check packet */
  if (!__is_valid_packet(socket,packet))
    return -L4_EINVAL;
#endif

  nr = packet->sg_idx;	/* prevent our peer from chaning the index while
  			   we are using it */
  			   
  if(nr < socket->num_sg_elems){
    l4_uint32_t flags;

    // normal case: we have a valid packet
    /* get data */
    *size = socket->sg_lists[nr].size;
    packet->sg_idx = socket->sg_lists[nr].next;
    	
    flags = socket->sg_lists[nr].flags & (DSI_DATA_AREA_GAP |
					  DSI_DATA_AREA_EOS |
	                                  DSI_DATA_AREA_PHYS);
    if(flags & DSI_DATA_AREA_PHYS)
	{
	*addr = (void*)socket->sg_lists[nr].addr;
	return -DSI_EPHYS;
	}
    else
	{
	*addr = socket->data_area + socket->sg_lists[nr].addr;
	if(!flags) return 0;
	if(flags & DSI_DATA_AREA_GAP) return -DSI_EGAP;
	return -DSI_EEOS;
	}
  } // normal case, packet had valid sg-number
  
  /* check if packet contains more data */
  if (packet->sg_idx == DSI_SG_ELEM_LAST)
    return -DSI_ENODATA;
  return -L4_EINVAL;
}

/*****************************************************************************/
/**
 * \brief Set packet number.
 * \ingroup packet
 * 
 * \param socket         Socket descriptor
 * \param packet         Packet descriptor
 * \param np             Packet number
 *
 * \return 0 on success, error code otherwise:
 *         - -L4_EINVAL invalid socket or packet descriptor
 *
 * \note The packet number can be any desired number that makes the packet
 *	 kind of unique.
 */
/*****************************************************************************/ 
int 
dsi_packet_set_no(dsi_socket_t * socket, dsi_packet_t * packet,
		  l4_uint32_t no)
{
#if DO_SANITY
  /* check socket */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* check packet */
  if (!__is_valid_packet(socket,packet))
    return -L4_EINVAL;

  /* only send components can set packet no */
  if (!IS_SEND_SOCKET(socket))
    return -L4_EINVAL;
#endif

  /* set packet no */
  packet->no = no;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Get packet number
 * \ingroup packet
 * 
 * \param  socket        Socket descriptor
 * \param  packet        Packet descriptor
 * \retval no            Packet number
 * 
 * \return 0 on success (\a no contains packet number), error code otherwise:
 *         - -L4_EINVAL invalid socket or packet descriptor
 */
/*****************************************************************************/ 
int 
dsi_packet_get_no(dsi_socket_t * socket, dsi_packet_t * packet,
		  l4_uint32_t * no)
{
#if DO_SANITY
  /* check socket */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* check packet */
  if (!__is_valid_packet(socket,packet))
    return -L4_EINVAL;
#endif

  *no = packet->no;

  /* done */
  return 0;
}
