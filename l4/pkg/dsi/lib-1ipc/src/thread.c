/* $Id$ */
/*****************************************************************************/
/**
 * \file  dsi/lib/src/thread.c
 * \brief Thread handling.
 *
 * \date   07/09/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/* lib includes */
#include <l4/dsi/dsi.h>
#include "__socket.h"
#include "__sync.h"
#include "__config.h"
#include "__debug.h"
#include "__thread.h"

static int sync_thread_prio = L4THREAD_DEFAULT_PRIO;
static int select_thread_prio = L4THREAD_DEFAULT_PRIO;
static int event_thread_prio = L4THREAD_DEFAULT_PRIO;

/*****************************************************************************/
/**
 * \brief Set priority of syncronisation threads created in the future.
 * \ingroup general
 *
 * \param new_prio	The new L4 priority. Use L4THREAD_DEFAULT_PRIO to
 *			use the default priority of the l4 thread lib. The
 *			actual priority depends on the implementation of
 *			the l4 thread lib.
 *
 * \return old sync-thread priority.
 *
 * This function sets the priority used to create the synchronisation threads.
 * This function does not effect existing threads.
 *
 * Prior to the first call, L4THREAD_DEFAULT_PRIO will be used to create
 * the synchronisation threads.
 */
/*****************************************************************************/ 
int
dsi_set_sync_thread_prio(int new_prio)
{
  int old_prio = sync_thread_prio;
  
  sync_thread_prio = new_prio;

  return old_prio;
}

/*****************************************************************************/
/**
 * \brief Set priority of the select threads created in the future.
 * \ingroup general
 *
 * \param new_prio	The new L4 priority. Use L4THREAD_DEFAULT_PRIO to
 *			use the default priority of the l4 thread lib. The
 *			actual priority depends on the implementation of
 *			the l4 thread lib.
 *
 * \return old select-thread priority.
 *
 * This function sets the priority used to create the select threads.
 * This function does not effect existing threads.
 *
 * Prior to the first call, L4THREAD_DEFAULT_PRIO will be used to create
 * the select threads.
 */
/*****************************************************************************/ 
int
dsi_set_select_thread_prio(int new_prio)
{
  int old_prio = select_thread_prio;
  
  select_thread_prio = new_prio;

  return old_prio;
}

/*****************************************************************************/
/**
 * \brief Set priority of event thread created in the future.
 * \ingroup general
 *
 * \param new_prio	The new L4 priority. Use L4THREAD_DEFAULT_PRIO to
 *			use the default priority of the l4 thread lib. The
 *			actual priority depends on the implementation of
 *			the l4 thread lib.
 *
 * \return old event-thread priority.
 *
 * This function sets the priority used to create the event thread.
 * It should be called prior to dsi_init(), otherwise it has no effect.
 *
 * If this function is not called, L4THREAD_DEFAULT_PRIO will be used to
 * create the event thread.
 */
/*****************************************************************************/ 
int
dsi_set_event_thread_prio(int new_prio)
{
  int old_prio = event_thread_prio;
  
  event_thread_prio = new_prio;

  return old_prio;
}


/*****************************************************************************/
/**
 * \brief Create new synchronization thread for socket.
 * \ingroup internal
 *
 * \param socket   Socket descriptor 
 * \return 0 on success, error code otherwise (see l4thread_create_long())
 *
 * The priority of the newly created thread can be set with
 * dsi_set_sync_thread_prio() prior to calling this function.
 */
/*****************************************************************************/ 
int
dsi_create_sync_thread(dsi_socket_t * socket)
{
  l4thread_t t;

  /* start synchronization thread */
  if (IS_SEND_SOCKET(socket))
    {
#if DEBGU_THREAD
      INFO("send sync thread...\n");
#endif
      t = l4thread_create_long(L4THREAD_INVALID_ID,dsi_sync_thread_send,
			       L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			       sync_thread_prio, (void *)socket,
			       L4THREAD_CREATE_ASYNC);
    }
  else
    {
#if DEBUG_THREAD
      INFO("receive sync thread...\n");
#endif
      t = l4thread_create_long(L4THREAD_INVALID_ID,dsi_sync_thread_receive,
			       L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			       sync_thread_prio, (void *)socket,
			       L4THREAD_CREATE_ASYNC);
    }

  if (t < 0)
    {
      Error("DSI: failed to create sync thread: %s (%d)",l4env_errstr(t),t);
      return t;
    }

#if DEBUG_THREAD
  INFO("created sync thread %d\n",t);
#endif

  /* setup socket descriptor */
  socket->sync_id = t;
  socket->sync_th = l4thread_l4_id(t);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Shutdown synchronization thread.
 * \ingroup internal
 * 
 * \param socket Socket descriptor.
 * \return 0 on success, error code otherwise (see l4thread_shutdown())
 */
/*****************************************************************************/ 
int
dsi_shutdown_sync_thread(dsi_socket_t * socket)
{
#if DEBUG_THREAD
  INFO("shutdown sync thread...\n");
#endif

  /* shutdown synchronization thread */
  return l4thread_shutdown(socket->sync_id);
}

/*****************************************************************************/
/**
 * \brief Start event signalling thread.
 * \ingroup internal
 *
 * \param  fn            Signalling thread function
 *	
 * \return L4 threadid of signalling thread, \c L4_INVALID_ID if creation 
 *         failed.
 *
 * The priority of the newly created thread can be set with
 * dsi_set_event_thread_prio() prior to calling this function.
 */
/*****************************************************************************/ 
l4_threadid_t 
dsi_create_event_thread(l4thread_fn_t fn)
{
  l4thread_t t;

  /* create thread */
  t = l4thread_create_long(L4THREAD_INVALID_ID,fn,
			   L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			   event_thread_prio,NULL,L4THREAD_CREATE_ASYNC);
  if (t < 0)
    {
      Panic("DSI: create event signalling thread failed: %s (%d)",
	    l4env_errstr(t),t);
      return L4_INVALID_ID;
    }
  else
    return l4thread_l4_id(t);
}

/*****************************************************************************/
/**
 * \brief Start a select thread.
 * 
 * \param  fn            Thread function
 * \param  data          Thread data
 *	
 * \return Thread id of select thread, \c L4THREAD_INVALID_ID if creation
 *         failed.
 *
 * The priority of the newly created thread can be set with
 * dsi_set_select_thread_prio() prior to calling this function.
 *
 */
/*****************************************************************************/ 
l4thread_t
dsi_create_select_thread(l4thread_fn_t fn, void * data)
{
  l4thread_t t;

  /* create thread */
  t = l4thread_create_long(L4THREAD_INVALID_ID,fn,L4THREAD_INVALID_SP,
			   L4THREAD_DEFAULT_SIZE,
			   select_thread_prio,
			   data,L4THREAD_CREATE_ASYNC);
  if (t < 0)
    {
      Error("DSI: create select thread failed: %s (%d)",
	    l4env_errstr(t),t);
      return L4THREAD_INVALID_ID;
    }
  else
    return t;
}

/*****************************************************************************/
/**
 * \brief Shutdown select thread.
 * 
 * \param  thread        Select thread.
 */
/*****************************************************************************/ 
void
dsi_shutdown_select_thread(l4thread_t thread)
{
  int ret;
 
  /* shutdown thread */
  ret = l4thread_shutdown(thread);

  if (ret)
    Error("DSI: shutdown select thread failed: %s (%d)\n",
	  l4env_errstr(ret),ret);
}
