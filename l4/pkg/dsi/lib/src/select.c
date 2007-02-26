/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/src/select.c
 * \brief  Event select implementation.
 *
 * \date   02/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>

/* library includes */
#include <l4/dsi/dsi.h>
#include "__event.h"
#include "__stream.h"
#include "__thread.h"
#include "__debug.h"

/*****************************************************************************
 *** global stuff
 *****************************************************************************/

/**
 * Select thread arguments 
 */
typedef struct dsi_select_thread_arg
{
  /* in */
  l4_uint32_t         events;  ///< event mask (wait request)
  dsi_socket_ref_t *  socket;  ///< component socket reference
  l4semaphore_t *     sem;     ///< select semaphore

  /* out */
  l4_uint32_t         mask;    ///< event mask set by the component
  int                 error;   ///< error code 
} dsi_select_thread_arg_t;

/*****************************************************************************/
/**
 * \brief Select thread 
 * \ingroup internal
 *
 * \param  data          Thread data, pointer to dsi_select_thread_arg struct
 */
/*****************************************************************************/ 
static void
__select_thread(void * data)
{
  dsi_select_thread_arg_t * args = (dsi_select_thread_arg_t *)data;
  l4_int32_t mask;

#if DEBUG_SELECT
  INFO("signalling thread %x.%x\n",
       args->socket->event_th.id.task,args->socket->event_th.id.lthread);
  INFO("socket %d, events 0x%08x\n",args->socket->socket,args->events);
#endif

  /* wait for event notification */
  mask = dsi_event_wait(args->socket->event_th,args->socket->socket,
			args->events);
#if DEBUG_SELECT
  INFO("mask = 0x%08x\n",mask);
#endif

  if (mask < 0)
    args->error = mask;
  else
    {
      args->mask = mask;
      args->error = 0;
    }

  /* wakeup client thread */
  l4semaphore_up(args->sem);

  /* sleep
   * FIXME: implement sleep never in thread lib, for now we just sleep 
   *        5 minutes an hope we are destroyed by the client thread 
   *        during that time.
   */
  l4thread_sleep(300000);
}

/*****************************************************************************/
/**
 * \brief Wait for component events.
 * \ingroup stream
 * 
 * \param  sockets       Socket list, the struct must contain:
 *                       - \c stream Stream descriptor
 *                       - \c component Component (either 
 *                                      \c DSI_SEND_COMPONENT or
 *                                      \c DSI_RECEIVE_COMPONENT)
 *                       - \c event Event mask
 * \param  num_sockets   Number of sockets
 * \retval events        Event list, it contains all sockets for which an 
 *                       event was received, the elements are:
 *                       - \c stream    Stream descriptor
 *                       - \c component Component
 *                       - \c mask      Event mask set by the component
 * \retval num_events    Number of sockets for which an event was received.
 *	
 * \return 0 if one of the events was set by the component, 
 *         error code otherwise:
 *         - \c -L4_EINVAL     invalid stream descriptor
 *         - \c -L4_ENOTHREAD  create select thread failed
 *         - \c -L4_EBUSY      someone else already waiting for one of the
 *                             events
 *         - \c -L4_EIPC       IPC error calling component
 *
 * Wait for component events. \a sockets contains a list of sockets and
 * events masks. For each of these sockets a thread is created with
 * dsi_create_select_thread(), which calls the components event signalling
 * thread to wait for an event notification.  If the first notification is
 * received (it can be immediately if one of the events is already set by
 * the component), the event is marked in the socket list, all threads are
 * destroyed and the function returns.
 */
/*****************************************************************************/ 
int
dsi_stream_select(dsi_select_socket_t *sockets, const int num_sockets,
		  dsi_select_socket_t *events, int * num_events)
{
  dsi_select_thread_arg_t args[num_sockets];
  l4thread_t threads[num_sockets];
  l4semaphore_t sem = L4SEMAPHORE_LOCKED;
  int i,j,error,ret;

  /* setup thread arguments */
  for (i = 0; i < num_sockets; i++)
    {
      if (!dsi_is_valid_stream(sockets[i].stream))
	return -L4_EINVAL;

      args[i].events = sockets[i].events;
      if (sockets[i].component & DSI_SEND_COMPONENT)	
	args[i].socket = &sockets[i].stream->sender.socketref;
      else
	args[i].socket = &sockets[i].stream->receiver.socketref;
      args[i].sem = &sem;
      args[i].mask = 0;
      args[i].error = 0;
    }

  /* start threads */
  for (i = 0; i < num_sockets; i++)
    {
      threads[i] = dsi_create_select_thread(__select_thread,&args[i]);
      if (threads[i] == L4THREAD_INVALID_ID)
	{
	  /* start thread failed, cleanup */
	  for (j = 0; j < i; j++)
	    dsi_shutdown_select_thread(threads[j]);
	  return -L4_ENOTHREAD;
	}
    }

  /* wait until an event is set by a component */
  l4semaphore_down(&sem);

  /* stop all threads, check error codes */
  error = 0;
  for (i = 0; i < num_sockets; i++)
    {
      /* shutdown thread */
      dsi_shutdown_select_thread(threads[i]);
      
      /* check error */
      error = args[i].error;
    }

  if (error)
    {
      Error("DSI: select error %d\n",error);
      return error;
    }

  /* setup return */
  j = 0;
  for (i = 0; i < num_sockets; i++)
    {
#if DEBUG_SELECT
      INFO ("%d, mask=0x%08x, error = %d", i, args[i].mask, args[i].error);
#endif
      if (args[i].mask)
	{
	  /* create entry in event list */
	  events[j].stream = sockets[i].stream;
	  events[j].component = sockets[i].component;
	  events[j].events = args[i].mask;

#if DEBUG_SELECT
	  INFO("events %08x <= %08x\n", events[j].events, args[i].mask);
	  INFO("reset 0x%08x at %x.%x\n",
	       args[i].mask,args[i].socket->event_th.id.task,
	       args[i].socket->event_th.id.lthread);
#endif

	  /* reset events */
	  ret = dsi_event_reset(args[i].socket->event_th,
				args[i].socket->socket,args[i].mask);
	  if (ret)
	    {
	      Error("DSI: reset events failed: %s (%d)\n",
		    l4env_errstr(ret),ret);
	    }
	  j++;	  
	}	  
    }

  *num_events = j;

  /* done */
  return error;
}
