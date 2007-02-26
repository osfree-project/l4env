/* $Id$ */
/*****************************************************************************/
/**
 * \file dsi/lib/src/socket.c
 *
 * \brief Component interface, socket implementation
 *
 * \author      Lars Reuther <reuther@os.inf.tu-dresden.de>
 * \date        07/01/2000
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>

/* library includes */
#include <l4/dsi/dsi.h>
#include "__socket.h"
#include "__dataspace.h"
#include "__thread.h"
#include "__sync.h"
#include "__event.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 * global types/structures
 *****************************************************************************/

/***
 * \brief field of socket descriptors
 */
static dsi_socket_t sockets[DSI_MAX_SOCKETS];

/**
 * \brief next index to search for unused socket
 * \ingroup internal
 */
static int next_socket = 0;

/**
 * socket->flags contains various types of flags, the upper 4 bits are 
 * used for internal stuff.
 */
#define SOCKET_FLAGS_USER     0x0FFFFFFFUL

/*****************************************************************************
 * helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Initialize socket table
 * \ingroup internal
 */
/*****************************************************************************/
void
dsi_init_sockets(void)
{
  int i;

  for (i = 0; i < DSI_MAX_SOCKETS; i++)
    sockets[i].flags = DSI_SOCKET_UNUSED;
}

/*****************************************************************************/
/**
 * \brief Find and allocate unused socket in socket table
 * \ingroup internal
 *
 * \return index of socket in socket table, -1 if no socket available
 */
/*****************************************************************************/ 
static int
__allocate_socket(void)
{
  int i = next_socket;

  /* search unused socket descriptor */
  do
    {
      if (cmpxchg32(&sockets[i].flags,DSI_SOCKET_UNUSED,DSI_SOCKET_USED))
	{
	  /* found */
	  next_socket = (i + 1) % DSI_MAX_SOCKETS;
	  break;
	}
      
      i = (i + 1) % DSI_MAX_SOCKETS;
    }
  while (i != next_socket);

  if (i == next_socket)
    return -1;
  else
    return i;
}

/*!\brief Check if socket is valid socket descriptor.
 * \ingroup component
 *
 * \param socket	socket descriptor
 * \retval		!= 0 if socket points to valid socket descriptor
 * \retval		0 otherwise
 */
int
dsi_is_valid_socket(dsi_socket_t * socket)
{
#if 0
  LOGdL(DEBUG_SOCKET,"socket at 0x%08x",(dword_t) socket);
#endif

  if (socket == NULL)
    return 0;

  return ((socket >= sockets) &&
	  (socket < &sockets[DSI_MAX_SOCKETS]) &&
	  (socket->flags != DSI_SOCKET_UNUSED));
}

/*!\brief Create new send/receive socket.
 * \ingroup socket
 *
 * This function allocates the necessary dataspaces, maps them, and creates
 * the synchronisation thread using dsi_create_sync_thread(). The dataspace
 * for the data area can be defined by the caller. Specify DSI_PACKET_MAP to
 * indicate a preallocated dataspace.  If no (invalid) control area or
 * synchronization thread are specified, they are created.
 *
 * \param jcp_stream	stream description
 * \param cfg		low level stream configuration
 * \param ctrl_ds	control area (if invalid, allocate new area)
 * \param data_ds	data area
 * \param work_id	work thread id
 * \param sync_id	synchronization thread id (if invalid, create thread)
 * \param flags		socket flags:
 *			- DSI_SOCKET_SEND	create send socket
 *			- DSI_SOCKET_RECEIVE	create receive socket
 *			- DSI_SOCKET_BLOCK	block if no packet is available
 *						in dsi_packet_get, the default 
 *                                              behavior is to return an error
 *		        - for more see include/l4/dsi/types.h
 *
 * \retval ctrl_ds	contains control area
 * \retval sync_id	synchronization thread id of newly created thread
 * \retval socket	socket descriptor
 * \retval 0		success, created socket
 * \retval -L4_EINVAL   invalid data area or work thread
 * \retval -L4_ENOSOCKET no socket descriptor available
 */
int
dsi_socket_create(dsi_jcp_stream_t jcp_stream, dsi_stream_cfg_t cfg,
		  l4dm_dataspace_t * ctrl_ds, l4dm_dataspace_t * data_ds,
		  l4_threadid_t work_id, l4_threadid_t * sync_id,
		  l4_uint32_t flags, dsi_socket_t ** socket)
{
  dsi_socket_t * s;
  int sid,ret,i;

  *socket = NULL;

  /* sanity checks */
  if (l4dm_is_invalid_ds(*data_ds))
    {
      Error("DSI: invalid data area!");
      return -L4_EINVAL;
    }
  LOGdL(DEBUG_SOCKET,"data_ds: %d at %t\n", data_ds->id, data_ds->manager);

  if (l4_is_invalid_id(work_id))
    {
      Error("DSI: invalid work thread");
      return -L4_EINVAL;
    }

  /* allocate new socket descriptor */
  sid = __allocate_socket();
  if (sid == -1)
    {
      /* no socket descriptor available */
      Error("DSI: no socket available");
      return -DSI_ENOSOCKET;
    }
  s = &sockets[sid];
  s->socket_id = sid;

  /* set user flags */
  s->flags |= (flags & SOCKET_FLAGS_USER);

  /* setup synchronization thread */
  if (l4_is_invalid_id(*sync_id))
    {
      /* create new synchronization thread */
      ret = dsi_create_sync_thread(s);
      if (ret)
	{
	  /* creation failed, release socket */
	  Error("DSI: create synchronization thread failed: %s (%d)",
		l4env_errstr(ret),ret);
	  s->flags = DSI_SOCKET_UNUSED;
	  return ret;
	}
      s->flags |=  DSI_SOCKET_FREE_SYNC;
      *sync_id = s->sync_th;
    }
  else
    /* use existing synchronization thread */
    s->sync_th = *sync_id;

  /* setup control area */
  if (l4dm_is_invalid_ds(*ctrl_ds))
    {
      /* create new one */
      LOGdL(DEBUG_SOCKET,"create control area");

      ret = dsi_create_ctrl_area(s,jcp_stream,cfg);
      if(ret)
	{
	  Error("DSI: error creating control area: %s (%d)",
		l4env_errstr(ret),ret);
	  return ret;
	}
      s->flags |= DSI_SOCKET_FREE_CTRL;
      *ctrl_ds = s->ctrl_ds;

      LOGdL(DEBUG_SOCKET,"control dataspace: %d at %t", 
            ctrl_ds->id, ctrl_ds->manager);
    }
  else 
    {
      /* use existing */
      ret = dsi_set_ctrl_area(s,*ctrl_ds,jcp_stream,cfg);
    }
  
  if (ret)
    {
      /* creation/setup failed, shutdown sync thread, release socket */
      Error("DSI: create/setup control area failed: %s (%d)",
	    l4env_errstr(ret),ret);
      if (s->flags & DSI_SOCKET_FREE_SYNC)
	dsi_shutdown_sync_thread(s);
      s->flags = DSI_SOCKET_UNUSED;
      return ret;
    }

  /* setup data area */
  LOGdL(DEBUG_SOCKET,"setting data area (%d at %t)...", 
        data_ds->id, data_ds->manager);

  ret = dsi_set_data_area(s,*data_ds);
  if (ret)
    {
      /* setup failed */
      Error("DSI: setup data area failed: %s (%d)",l4env_errstr(ret),ret);
      if (s->flags & DSI_SOCKET_FREE_SYNC)
	dsi_shutdown_sync_thread(s);
      if (s->flags & DSI_SOCKET_FREE_CTRL)
	dsi_release_ctrl_area(s);
      s->flags = DSI_SOCKET_UNUSED;
      return ret;
    }

  /* reset event counter */
  for (i = 0; i < DSI_MAX_EVENTS; i++)
    s->events[i] = 0;

  /* finish socket setup */
  s->work_th = work_id;
  s->sync_callback = NULL;
  s->release_callback = NULL;
  s->packet_count = 0;
  s->next_packet = 0;
  s->next_sg_elem = 0;
  s->waiting = 0;
  s->clients = NULL;

  /* done */
  *socket = s;

  return 0;
}

/*!\brief Close a socket.
 * \ingroup socket
 *
 * This function stops the synchronisation thread and frees the allocated
 * dataspaces. If the data dataspace was allocated prior to calling
 * dsi_socket_create(), it will not be deleted.
 *
 * \param socket	socket descriptor
 * \retval 0		success, closed socket
 * \retval -L4_EINVAL   invalid socket descriptor
 *
 */
int
dsi_socket_close(dsi_socket_t * socket)
{
  /* sanity check */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* shutdown synchronization thread */
  if (socket->flags & DSI_SOCKET_FREE_SYNC)
    dsi_shutdown_sync_thread(socket);

  /* detach/close control dataspace */
  dsi_release_ctrl_area(socket);

  /* detach data dataspace */
  dsi_release_data_area(socket);

  /* mark socket unused */
  socket->flags = DSI_SOCKET_UNUSED;

  /* done */
  return 0;
}

/*!\brief Stop a socket.
 * \ingroup socket
 *
 * This function stops the synchronisation thread. This ensures that
 * the synchronization thread does not touch the control area any
 * longer. This would be a problem if the sender sends a
 * commit-message and destroys the control area afterwards: The
 * receiver is scheduled (depending on the priorities), sees the received
 * message and tries to access the (unmapped) control area.
 *
 * \param socket	socket descriptor
 * \retval 0		success, closed socket
 * \retval -L4_EINVAL   invalid socket descriptor
 * */
int
dsi_socket_stop(dsi_socket_t * socket)
{
  /* sanity check */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* shutdown synchronization thread */
  if (socket->flags & DSI_SOCKET_FREE_SYNC)
    dsi_shutdown_sync_thread(socket);
  socket->flags &= ~DSI_SOCKET_FREE_SYNC;

  /* done */
  return 0;
}

/*!\brief Connect the socket to a partner
 * \ingroup socket
 *
 * This function enters the data describing a remote endpoint into the socket
 * structure. No start IPC or similar things are performed here.
 *
 * \param socket	local socket descriptor
 * \param remote_socket	remote socket reference
 *
 * \retval 0		success, socket connected to remote socket
 * \retval -L4_EINVAL   invalid socket reference
 *
 * \see dsi_socket_connect_ref().
 */
int 
dsi_socket_connect(dsi_socket_t * socket, dsi_socket_ref_t * remote_socket)
{
  int ret;

  /* sanity check */
  if (!dsi_is_valid_socket(socket))
    {
      Error("DSI: invalid socket");
      return -L4_EINVAL;
    }
  
  if (l4_is_invalid_id(remote_socket->work_th) || 
      l4_is_invalid_id(remote_socket->sync_th) || 
      !l4_task_equal(remote_socket->work_th,remote_socket->sync_th))
    {
      printf("work %x.%x, sync %x.%x\n",
             remote_socket->work_th.id.task,remote_socket->work_th.id.lthread,
             remote_socket->sync_th.id.task,remote_socket->sync_th.id.lthread);
      Error("DSI: invalid data in socket descriptor");
      return -L4_EINVAL;
    }

  LOGdL(DEBUG_CONNECT,"connecting socket %d",socket->socket_id);
  LOGdL(DEBUG_CONNECT,"remote: %d, %x.%x, %x.%x",
       remote_socket->socket,
       remote_socket->work_th.id.task, remote_socket->work_th.id.lthread,
       remote_socket->sync_th.id.task, remote_socket->sync_th.id.lthread);

  /* set remote socket */
  socket->remote_socket.socket = remote_socket->socket;
  socket->remote_socket.work_th = remote_socket->work_th;
  socket->remote_socket.sync_th = remote_socket->sync_th;

  /* wakeup synchronization thread */
  ret = dsi_start_sync_thread(socket);
  if (ret)
    {
      Error("DSI: connect: wakeup synchronization thread failed!");
      return ret;
    }

  /* done */
  return 0;
}

/*!\brief Set callback for syncronisation events.
 * \ingroup socket
 *
 * The specified function will be called if synchronisation IPC is necessary
 * due to blocking. The call will be performed inside packet_get(),
 * immediately before the request for sync-IPC is sent to the peer.
 *
 * \param socket	socket descriptor
 * \param func		synchronization callback function
 *
 * \retval 0		success, registered new callback function
 * \retval -L4_EINVAL   invalid socket descriptor / callback function
 */
int
dsi_socket_set_sync_callback(dsi_socket_t * socket, 
			     dsi_sync_callback_fn_t func)
{
  /* sanity check */
  if (!dsi_is_valid_socket(socket) || (func == NULL))
    return -L4_EINVAL;

  /* set callback function */
  socket->sync_callback = func; 
  socket->flags |= DSI_SOCKET_SYNC_CALLBACK;

  /* done */
  return 0;
}

/*!\brief Set callback for release events.
 * \ingroup socket
 *
 * The specified function will be called if the release event is triggered.
 * The release event will be triggered when the partner commits a packet
 * with dsi_packet_commit() and release notification is set for this packet
 * or the entire stream (flag #DSI_PACKET_RELEASE_CALLBACK).
 *
 * \param socket	socket descriptor
 * \param func		packet release callback function
 *
 * \retval 0		success, registered new callback function
 * \retval -L4_EINVAL   invalid socket descriptor / callback function
 */
int
dsi_socket_set_release_callback(dsi_socket_t * socket,
				dsi_release_callback_fn_t func)
{
  /* sanity check */
  if (!dsi_is_valid_socket(socket) || (func == NULL))
    return -L4_EINVAL;

  /* set callback function */
  socket->release_callback = func;
  socket->flags |= DSI_SOCKET_RELEASE_CALLBACK;

  /* done */
  return 0;
}

/*!\brief Obtain a reference to a socket
 * \ingroup socket
 *
 * This function returns a reference to a socket which can be passed to
 * another task. The other task can use the reference to communicate with this
 * socket (see dsi_socket_connect()).
 *
 * \param socket	socket descriptor
 *
 * \retval ref		contains socket reference
 * \retval 0		success
 * \retval -L4_EINVAL   invalid socket descriptor
 */
int
dsi_socket_get_ref(dsi_socket_t * socket, dsi_socket_ref_t * ref)
{
  /* sanity check */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;
  
  /* create external reference */
  ref->socket = socket->socket_id;
  ref->work_th = socket->work_th;
  ref->sync_th = socket->sync_th;
  ref->event_th = dsi_get_event_thread_id();

  return 0;
}

/*!\brief Get the socket of a given socket-ID.
 * \ingroup socket
 *
 * To get the socket of a socket reference \c x (\a dsi_socket_ref_t), use
 * \c dsi_socket_get_descriptor(x.socket,&s). Note, the socket
 * reference must refer to a socket in the own task.
 *
 * \param id		socket id
 *
 * \retval socket     contains the socket
 * \retval 0	      success
 * \retval -L4_EINVAL invalid socket id
 */
int
dsi_socket_get_descriptor(dsi_socketid_t id, dsi_socket_t ** socket)
{
  /* check socket id */
  if ((id < 0) || (id >= DSI_MAX_SOCKETS))
    return -L4_EINVAL;

  if (sockets[id].flags == DSI_SOCKET_UNUSED)
    return -L4_EINVAL;

  /* return socket descriptor */
  *socket = &sockets[id];
  return 0;
}

/*!\brief Return start address and size of data area.
 * \ingroup socket
 *        
 * On socket creation, the data area is mapped into the local address space.
 * Use this function should be used to get a pointer to the mapped area.
 *
 * \param socket	socket descriptor
 *
 * \retval data_area  start address of data area
 * \retval area_size  size of data area
 * \retval 0	      success
 * \retval -L4_EINVAL invalid socket descriptor
 */
int
dsi_socket_get_data_area(dsi_socket_t * socket, void **data_area,
			 l4_size_t * area_size)
{
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* return data area start address */
  *data_area = socket->data_area;
  *area_size = socket->data_size;

  return 0;
}

/*****************************************************************************/
/**
 * \brief Set socket flags.
 * \ingroup socket
 * 
 * \param  socket        Socket descriptor
 * \param  flags         Flags to set
 *	
 * \return 0 on success (modified socket flags), error code otherwise:
 *         - \c -L4_EINVAL  invalid socket descriptor
 */
/*****************************************************************************/ 
int
dsi_socket_set_flags(dsi_socket_t * socket, l4_uint32_t flags)
{
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* modify flags */
  socket->flags |= (flags & DSI_SOCKET_USER_FLAGS);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Clear socket flags.
 * \ingroup socket
 * 
 * \param  socket        Socket descriptor
 * \param  flags         Flags to clear
 *	
 * \return 0 on success (modified socket flags), error code otherwise:
 *         - \c -L4_EINVAL  invalid socket descriptor
 */
/*****************************************************************************/ 
int
dsi_socket_clear_flags(dsi_socket_t * socket, l4_uint32_t flags)
{
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  /* modify flags */
  socket->flags &= ~(flags & DSI_SOCKET_USER_FLAGS);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Test socket flag.
 * \ingroup socket
 * 
 * \param  socket        Socket descriptor
 * \param  flag          Flag
 *	
 * \return != 0 if flag is set, 0 if flag is not set or invalid socket 
 *         descriptor
 */
/*****************************************************************************/ 
int
dsi_socket_test_flag(dsi_socket_t * socket, l4_uint32_t flag)
{
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return 0;
  
  /* test flag */
  return (socket->flags & (flag & DSI_SOCKET_USER_FLAGS));
}

/*****************************************************************************/
/**
 * \brief Set event
 * \ingroup socket
 * 
 * \param  socket        Socket descriptor
 * \param  events        Event mask
 *	
 * \return 0 on success (called event signalling thread), error code otherwise:
 *         - \c -L4_EINVAL  invalid socket descriptor
 *         - \c -L4_EIPC    IPC error calling signalling thread
 *
 * This function queues a given event for the socket. The event can be
 * waited for at the client application using the dsi_event_wait()
 * function.
 *
 * This function just calls dsi_event_set().
 */
/****************************************************************************/ 
int 
dsi_socket_set_event(dsi_socket_t * socket, l4_uint32_t events)
{
  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  LOGdL(DEBUG_EVENT,"set events 0x%08x",events);

  /* set event */
  return dsi_event_set(socket->socket_id,events);
}

