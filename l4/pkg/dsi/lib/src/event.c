/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/src/event.c
 * \brief  Event signalling thread
 *
 * \date   02/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
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
#include "__event.h"
#include "__thread.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global stuff
 *****************************************************************************/

/// component event signalling thread id
static l4_threadid_t dsi_component_event_id = L4_INVALID_ID;

#define MAX_CLIENTS  (DSI_MAX_EVENTS * DSI_MAX_SOCKETS)

/// client descriptors 
static dsi_event_client_t clients[MAX_CLIENTS];

/// IPC commands 
#define EVENT_SET    0x00000000
#define EVENT_RESET  0x40000000
#define EVENT_WAIT   0x80000000

#define EVENT_MASK   ((1UL << DSI_MAX_EVENTS) - 1)

/* prototypes */
static void
__event_thread(void * data);

/*****************************************************************************/
/**
 * \brief Init event signalling.
 * \ingroup internal
 */
/*****************************************************************************/ 
void
dsi_init_event_signalling(void)
{
  int i;

  /* start signalling thread */
  dsi_component_event_id = dsi_create_event_thread(__event_thread);

  for (i = 0; i < MAX_CLIENTS; i++)
    clients[i].events = 0;
}

/*****************************************************************************
 *** Component event handling
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Send event notification to client application.
 * \ingroup internal
 * 
 * \param  client        Client thread id
 * \param  events        Event mask
 * \param  error         Error code to be sent to the client
 * 
 * \return 0 on success (sent notification), -L4_EIPC on error
 */
/*****************************************************************************/ 
static inline int
__client_wakeup(l4_threadid_t client, l4_uint32_t events, int error)
{
  int ret;
  l4_msgdope_t result;

  /* send notification */
  ret = l4_ipc_send(client,L4_IPC_SHORT_MSG,error,events,
			 L4_IPC_SEND_TIMEOUT_0,&result);
  if ((ret == 0) || (ret == L4_IPC_SETIMEOUT)) 
    return 0;
  else 
    {
      LOG_Error("DSI: send event notification failed (0x%02x)",ret);
      return -L4_EIPC;
    }
}
  
/*****************************************************************************/
/**
 * \brief Set events.
 * \ingroup internal
 *
 * \param  id            Socket id
 * \param  events        Event mask
 *	
 * \return 0 on success (incremented event counter), error code otherwise:
 *         - \c -L4_EINVAL  invalid socket id
 *         - \c -L4_EIPC    IPC error sending wakeup message
 *
 * Increment event counter for the vents specified in \a events and wakeup
 * clients waiting for those events.
 */
/*****************************************************************************/ 
static int
__set_event(dsi_socketid_t id, l4_uint32_t events)
{
  dsi_socket_t * s;
  int ret,i,error;
  dsi_event_client_t * c;
  dsi_event_client_t * tmp;

  LOGdL(DEBUG_EVENT,"socket %d, events 0x%08x",id,events);

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor(id,&s);
  if (ret)
    {
      LOG_Error("DSI: invalid socket id (%d)",id);
      return -L4_EINVAL;
    }

  /* increment event counter */
  for (i = 0; i < DSI_MAX_EVENTS; i++)
    {
      if (events & (1UL << i))
	/* set event i */
	s->events[i]++;
    }

  /* is somebody waiting for one of the events? */
  error = 0;
  if (s->waiting & events)
    {
      c = s->clients;
      tmp = NULL;
      while (c)
	{
	  if (c->events & events)
	    {
	      /* wakeup client */
	      ret = __client_wakeup(c->id,c->events & events,0);
	      if (ret)
		error = ret;

	      /* remove waiting flags */
	      s->waiting &= ~(c->events);

	      /* release client descriptor */
	      c->events = 0;

	      /* remove from wait queue */
	      if (tmp == NULL)
		{
		  s->clients = c->next; 
		  c = s->clients;
		}
	      else
		{
		  tmp->next = c->next;
		  c = c->next;
		}
	    }
	  else
	    {
	      tmp = c;
	      c = c->next;
	    }
	} 
    }

  /* done */
  return error;
}

/*****************************************************************************/
/**
 * \brief Reset events.
 * \ingroup internal
 * 
 * \param  client        Client thread id
 * \param  id            Socket id
 * \param  events        Event mask
 *	        
 * \return 0 on succes (decremented event counter), error code otherwise:
 *         - \c -L4_EINVAL  invalid socket id or event mask
 */
/*****************************************************************************/ 
static int 
__reset_event(l4_threadid_t client, dsi_socketid_t id, l4_uint32_t events)
{
  dsi_socket_t * s;
  int ret,i,error;

  LOGdL(DEBUG_EVENT,"socket %d, events 0x%08x",id,events);

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor(id,&s);
  if (ret)
    {
      LOG_Error("DSI: invalid socket id (%d)",id);
      return -L4_EINVAL;
    }

  /* increment event counter */
  error = 0;
  for (i = 0; i < DSI_MAX_EVENTS; i++)
    {
      if (events & (1UL << i))
	{
	  if (s->events[i] > 0)
	    s->events[i]--;
	  else
	    {
	      LOG_Error("DSI: event 0x%08x not set",1U << i);
	      error = -L4_EINVAL;
	    }
	}
    }

  /* done */
  return error;
}

/*****************************************************************************/
/**
 * \brief Wait for events.
 * \ingroup internal
 * 
 * \param  client        Client thread id
 * \param  id            Socket id
 * \param  events        Event mask
 *	
 * \return 0 on success (registered client), error code otherwise:
 *         - \c -L4_EINVAL  invalid socket id
 *         - \c -L4_EBUSY   someone else already registered for one of
 *                          the events
 *
 * Register the client for event notification. If one of the events is 
 * already set, wakeup client immediately.
 */
/*****************************************************************************/ 
static int
__wait_for_events(l4_threadid_t client, dsi_socketid_t id, l4_uint32_t events)
{
  dsi_socket_t * s;
  int ret,i;
  l4_uint32_t mask;
  dsi_event_client_t * c;

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor(id,&s);
  if (ret)
    {
      LOG_Error("DSI: invalid socket id (%d)",id);
      return -L4_EINVAL;
    }
  
  /* other clients already registered for one of the events? */
  if (s->waiting & events)
    return -L4_EBUSY;

  /* events already set? */
  mask = 0;
  for (i = 0; i < DSI_MAX_EVENTS; i++)
    {
      if ((events & (1U << i)) && (s->events[i] > 0))
	mask |= (1U << i);
    }

  if (mask)
    __client_wakeup(client,mask,0);
  else
    {
      /* enqueue client */
      i = 0;
      while ((i < MAX_CLIENTS) && (clients[i].events != 0))
	i++;

      if (i == MAX_CLIENTS)
	{
	  /* this should never happen, there should be enough descriptors 
	   * to store the maximum number of clients (one separate client
	   * for each possible event).
	   */
	  Panic("DSI: wait for event: no client descriptor available!");
	  return -L4_EINVAL;
	}

      /* setup client descriptor */
      clients[i].id = client;
      clients[i].events = events;
      clients[i].next = NULL;

      /* insert into sockest client wait queue */
      if (s->clients == NULL)
	s->clients = &clients[i];
      else
	{
	  c = s->clients;
	  while (c->next != NULL)
	    c = c->next;
	  c->next = &clients[i];
	}

      /* mark events waiting */
      s->waiting |= events;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Component event signalling thread.
 * \ingroup internal
 * 
 * \param  data          Thread data (unused).
 *
 * IPC protocol
 *
 * request:
 *   dw0 bits 31/30 command
 *                  0 ... set events (only allowed by other threads of the
 *                        component)
 *                  1 ... reset events
 *                  2 ... wait for events
 *       bits 29-0  event mask
 *   dw1 socket id
 *
 * reply:
 *   dw0 error code
 *   dw1 event mask (reply for EVENT_WAIT)
 */
/*****************************************************************************/ 
static void
__event_thread(void * data)
{
  l4_threadid_t me = l4thread_l4_id(l4thread_myself());
  l4_threadid_t src;
  int ret,error,reply;
  l4_uint32_t dw0,dw1;
  l4_msgdope_t result;
  int cmd;

  LOGdL(DEBUG_EVENT,"signalling thread up.");

  /* thread loop */
  while (1)
    {
      ret = l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,
			     L4_IPC_NEVER,&result);
      while (!ret)
	{
	  LOGdL(DEBUG_EVENT,"request: dw0 0x%08x, dw1 %d",dw0,dw1);

	  reply = 1;
	  cmd = dw0 & ~EVENT_MASK;

	  switch (cmd)
	    {
	    case EVENT_SET:
	      /* set events */
	      if (l4_task_equal(src,me))
		error = __set_event(dw1,dw0 & EVENT_MASK);
	      else
		error = -L4_EPERM;
	      break;

	    case EVENT_RESET:
	      /* reset events */
	      error = __reset_event(src,dw1,dw0 & EVENT_MASK);
	      break;

	    case EVENT_WAIT:
	      /* wait for events */
	      error = __wait_for_events(src,dw1,dw0 & EVENT_MASK);
	      if (!error)
		reply = 0;
	      break;

	    default:
	      /* invalid request */
	      LOG_Error("DSI: event signalling thread: invalid request 0x%08x",
                        dw0);
	      error = -L4_EINVAL;
	    }

	  if (reply)
	    ret = l4_ipc_reply_and_wait(src,L4_IPC_SHORT_MSG,error,0,
					     &src,L4_IPC_SHORT_MSG,&dw0,&dw1,
					     L4_IPC_SEND_TIMEOUT_0,
					     &result);
	  else
	    ret = l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,
				   L4_IPC_SEND_TIMEOUT_0,&result);
	}

      LOG_Error("DSI: event signalling thread IPC error 0x%02x",ret);
    }

  /* this should never happen */
  Panic("left event signalling thread!"); 
}

/*****************************************************************************/
/**
 * \brief Set events
 * \ingroup internal
 * 
 * \param  socket_id     Socket id
 * \param  events        Event mask
 *	
 * \return 0 on success (called signalling thread to set events), 
 *         error code otherwise:
 *         - \c -L4_EINVAL  invalid socket id
 *         - \c -L4_EIPC    IPC error sending wakeup message to client
 *
 * Call event signalling thread to set events. All the manipulation of the 
 * event counter and wait queues is done by the signalling thread to ensure
 * synchronization with client requests. 
 */
/*****************************************************************************/ 
int 
dsi_event_set(dsi_socketid_t socket_id, l4_uint32_t events)
{
  int ret,error,dummy;
  l4_msgdope_t result;

  /* call signalling thread */
  ret = l4_ipc_call(dsi_component_event_id,L4_IPC_SHORT_MSG,
			 EVENT_SET | (events & EVENT_MASK),socket_id,
			 L4_IPC_SHORT_MSG,&error,&dummy,
			 L4_IPC_NEVER,&result);
  if (ret)
    {
      LOG_Error("DSI: error calling event signalling thread (0x%02x)!",ret);
      return -L4_EIPC;
    }
 
  /* done */
  return error;
}

/*****************************************************************************/
/**
 * \brief Return id of event signalling thread.
 * \ingroup component 
 * 
 * \return Thread id.
 */
/*****************************************************************************/ 
l4_threadid_t
dsi_get_event_thread_id(void)
{
  /* return thread id */
  return dsi_component_event_id;
}

/*****************************************************************************
 *** Application event handling
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Reset events
 * \ingroup internal
 * 
 * \param  event_thread  Event signalling thread id
 * \param  socket_id     Socket id
 * \param  events        Event mask 
 *	
 * \return 0 on success (called components event signalling thread), 
 *         error code otherwise:
 *         - \c -L4_EIPC    error calling signalling thread
 *         - \c -L4_EINVAL  invalid socket id or event mask
 */
/*****************************************************************************/ 
int
dsi_event_reset(l4_threadid_t event_thread, dsi_socketid_t socket_id,
		l4_uint32_t events)
{
  int ret,error,dummy;
  l4_msgdope_t result;

  /* call component */
  ret = l4_ipc_call(event_thread,L4_IPC_SHORT_MSG,
			 EVENT_RESET | (events & EVENT_MASK),socket_id,
			 L4_IPC_SHORT_MSG,&error,&dummy,
			 L4_IPC_NEVER,&result);
  if (ret)
    {
      LOG_Error("DSI: error calling components event signalling thread (0x%02x)!",
                ret);
      return -L4_EIPC;
    }

  /* done */
  return error;
}

/*****************************************************************************/
/**
 * \brief Wait for events
 * \ingroup internal
 * 
 * \param  event_thread  Event signalling thread id
 * \param  socket_id     Socket id
 * \param  events        Event mask
 *	
 * \return Event mask set by the component (> 0), error code otherwise:
 *         - \c -L4_EIPC    error calling signalling thread
 *         - \c -L4_EINVAL  invalid socket id
 *         - \c -L4_EBUSY   another client is already registered for one
 *                          of the events
 *
 * Wait for the notification by the component that one of the events in 
 * \a events is set.
 */
/*****************************************************************************/ 
l4_int32_t
dsi_event_wait(l4_threadid_t event_thread, dsi_socketid_t socket_id,
	       l4_uint32_t events)
{
  int ret,error;
  l4_int32_t mask;
  l4_msgdope_t result;

  /* call component */
  ret = l4_ipc_call(event_thread,L4_IPC_SHORT_MSG,
			 EVENT_WAIT | (events & EVENT_MASK),socket_id,
			 L4_IPC_SHORT_MSG,&error,&mask,
			 L4_IPC_NEVER,&result);
  if (ret)
    {
      LOG_Error("DSI: error calling components event signalling thread (0x%02x)!",
                ret);
      return -L4_EIPC;
    }
     
  /* done */
  if (error)
    return error;
  else
    return mask;
}
