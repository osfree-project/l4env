/* $Id$ */
/*****************************************************************************/
/*!
 * \file    dsi/lib/src/app.c
 *
 * \brief   DRROPS Stream Interface. Application stream implementation.
 *
 * \date    07/10/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS inclues */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>

/* library includes */
#include <l4/dsi/dsi.h>
#include "__event.h"
#include "__stream.h"
#include "__config.h"
#include "__debug.h"
#include "__app.h"

/******************************************************************************
 * global structures                                                          *
 *****************************************************************************/

//! stream descriptors
static dsi_stream_t streams[DSI_MAX_STREAMS];

//! next index to search for unused stream
static int next_stream;

/*!\brief Initialize stream table.
 * \ingroup internal
 */
void
dsi_init_streams(void)
{
  int i;

  for (i = 0; i < DSI_MAX_STREAMS; i++)
    streams[i].flags = DSI_STREAM_UNUSED;
}

/*!\brief Find and allocate unused stream descriptor
 * \ingroup internal
 *
 * \return pointer to unused stream, \c NULL if no stream is available.
 */
static dsi_stream_t *
__allocate_stream(void)
{
  int i = next_stream;

  /* search unused stream descriptor */
  do
    {
      if (l4util_cmpxchg32(&streams[i].flags,
			   DSI_STREAM_UNUSED,DSI_STREAM_USED))
	{
	  /* found */
	  next_stream = (i + 1) % DSI_MAX_STREAMS;
	  break;
	}
      
      i = (i + 1) % DSI_MAX_STREAMS;
    }
  while (i != next_stream);

  if (i == next_stream)
    return NULL;
  else
    return &streams[i];
}

/*!\brief Check if \c stream is a valid stream descriptor.
 * \ingroup internal
 *
 * \param stream	stream descriptor
 * \retval "!0"		if stream points to valid stream descriptor
 * \retval 0		otherwise
 */
int 
dsi_is_valid_stream(dsi_stream_t * stream)
{
  if (stream == NULL)
    return 0;

  return ((stream >= streams) &&
	  (stream < &streams[DSI_MAX_STREAMS]) &&
	  (stream->flags != DSI_STREAM_UNUSED));
}

/*!\brief Check if \c component is a valid component decription.
 * \ingroup internal
 *
 * \param component	component description
 * \retval "!0"		if component is valid description
 * \retval 0		otherwise
 */
static int 
dsi_is_valid_component(dsi_component_t * component)
{
  if (component == NULL)
    return 0;

  return ((!l4_is_invalid_id(component->socketref.work_th)) &&
	  (!l4_is_invalid_id(component->socketref.sync_th)) &&
	  (component->connect != NULL));
}

/*!\brief Create application stream and connect send/receive component.
 * \ingroup stream
 *
 * \param sender	description of send component
 * \param receiver	description od receive component
 * \param ctrl		control area
 * \param data		data area
 *
 * \retval 0		  success, created stream, \c stream contains stream
 *			  descriptor
 * \retval -L4_EINVAL     invalid component / dataspace descriptors
 * \retval -DSI_ECONNECT  connect call to sender or receiver failed
 * \retval -DSI_ENOSTREAM no stream descriptor available
 */
int
dsi_stream_create(dsi_component_t * sender, dsi_component_t * receiver,
		  l4dm_dataspace_t ctrl, l4dm_dataspace_t data,
		  dsi_stream_t ** stream)
{
  int ret;
  dsi_stream_t * s;

  *stream = NULL;
 
  /* sanity checks */
  if (!dsi_is_valid_component(sender))
    {
      Error("DSI: invalid send component");
      return -L4_EINVAL;
    }
  
  if (!dsi_is_valid_component(receiver))
    {
      Error("DSI: invalid receive component");
      return -L4_EINVAL;
    }

  if (l4dm_is_invalid_ds(ctrl))
    {
      Error("DSI: invalid control area");
      return -L4_EINVAL;
    }

  if (l4dm_is_invalid_ds(data))
    {
      Error("DSI: invalid data area");
      return -L4_EINVAL;
    }

  /* allocate stream descriptor */
  s = __allocate_stream();
  if (s == NULL)
    {
      /* no stream descriptor available */
      Error("DSI: no stream descriptor available!");
      return -DSI_ENOSTREAM;
    }

  /* connect components */
  ret = sender->connect(sender,&receiver->socketref);
  if (ret)
    {
      Error("DSI: connect sender failed: %s (%d)",l4env_errstr(ret),ret);
      s->flags = DSI_STREAM_UNUSED;
      return -DSI_ECONNECT;
    }
  
  ret = receiver->connect(receiver,&sender->socketref);
  if (ret)
    {
      Error("DSI: connect receiver failed: %s (%d)",l4env_errstr(ret),ret);
      s->flags = DSI_STREAM_UNUSED;
      return -DSI_ECONNECT;
    }
  
  /* setup stream descriptor */
  s->sender = *sender;
  s->receiver = *receiver;
  s->ctrl = ctrl;
  s->data = data;
  s->__private = NULL;

  *stream = s;

  /* done */
  return 0;
}

/*!\brief Send start messages to send/receive components.
 * \ingroup stream
 *
 * \param stream      stream descriptor
 * \retval 0	      success
 * \retval -L4_EINVAL invalid stream descriptor
 *
 * Conceptionally, this function starts the data transmission on a
 * stream. Technically, this function just calls the start-functions
 * of the send and receive component of the stream. They should be
 * implemented in a way that data transmission is started this way. It
 * is perfectly legal for a send component to already start its send
 * operation after dsi_stream_create() to achieve its preload needed
 * for real-time guarantees. But the send component may stop generating
 * packets after it achieved its preload until dsi_stream_start() is called.
 */
int
dsi_stream_start(dsi_stream_t * stream)
{
  int ret;

  /* check stream descriptor */
  if (!dsi_is_valid_stream(stream))
    return -L4_EINVAL;

  /* send start message to receiver */
  if (stream->receiver.start != NULL)
    {
      ret = stream->receiver.start(&stream->receiver);
      if (ret)
	{
	  Error("DSI: start receiver failed: %s (%d)",
		l4env_errstr(ret),ret);
	  return ret;
	}
    }

  /* send start message to sender */
  if (stream->sender.start != NULL)
    {
      ret = stream->sender.start(&stream->sender);
      if (ret)
	{
	  Error("DSI: start sender failed: %s (%d)",
		l4env_errstr(ret),ret);
	  return ret;
	}
    }
  
  /* done */
  return 0;
}

/*!
 * \brief stop the transfer on a stream
 * \ingroup stream
 *
 * This function calls the stop-functions provided on dsi_stream_create()
 * to stop the data transfer. It is up to the two components to react in a
 * reasonable way, dsi does not check anything, it just calls the functions,
 * first sender, receiver then.
 *
 * \param stream stream descriptor
 * \retval 0		on success
 * \retval -L4_EINVAL	invalid stream descriptor
 */
int
dsi_stream_stop(dsi_stream_t * stream)
{
  int ret;

  // check stream descriptor
  if (!dsi_is_valid_stream(stream))
    return -L4_EINVAL;
	    
  /* send stop message to sender */
  LOGdL(DEBUG_STREAM,"calling sender (%x) to stop...",
        stream->sender.socketref.work_th.id.task);

  if (stream->sender.stop != NULL){
    ret = stream->sender.stop(&stream->sender);
    if (ret)
      {
        Error("DSI: stop sender failed: %s (%d)",l4env_errstr(ret),ret);
        return ret;
      }
  }

  /* send stop message to receiver */
  LOGdL(DEBUG_STREAM,"calling receiver (%x) to stop...",
        stream->receiver.socketref.work_th.id.task);

  if (stream->receiver.stop != NULL){
    ret = stream->receiver.stop(&stream->receiver);
    if (ret)
      {
        Error("DSI: stop receiver failed: %s (%d)",
              l4env_errstr(ret),ret);
        return ret;
      }
  }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Close stream
 * \ingroup stream
 * 
 * \param  stream        Stream descriptor
 *	
 * \return 0 on success (stream closed), error code otherwise:
 *         - \c -L4_EINVAL       invalid socket descriptor
 *         - \c -DSI_ECOMPONENT  component operation failed
 * 
 * We need two steps to close a stream:
 * 1. Stop the send and receive component. After the stop function returned
 *    we can be sure that a component stopped its processing and does not
 *    access stream data anymore.
 * 2. Close the sockets and the stream.  
 * These two steps are required to ensure that no resource are released in 
 * a component while the other component still uses them.
 */
/*****************************************************************************/ 
int
dsi_stream_close(dsi_stream_t * stream)
{
  int ret;
  int error = 0;

  // check stream descriptor
  if (!dsi_is_valid_stream(stream))
    return -L4_EINVAL;
  
  // we need at least the close functions for the sockets 
  if (stream->sender.close == NULL)
    {
      Error("DSI: missing send components close function");
      return -L4_EINVAL;
    }
  if (stream->receiver.close == NULL)
    {
      Error("DSI: missing receive components close function");
      return -L4_EINVAL;
    }

  // stop send component
  if (stream->sender.stop != NULL)
    {
      LOGdL(DEBUG_STREAM,"calling sender (%x) to stop...",
            stream->sender.socketref.work_th.id.task);

      ret = stream->sender.stop(&stream->sender);
      if (ret)
	{
	  Error("DSI: stop sender's failed: %s (%d)",l4env_errstr(ret),ret);
	  error = -DSI_ECOMPONENT;
	}
      LOGdL(DEBUG_STREAM,"sender stop returned");
    }

  // stop receive component
  if (stream->receiver.stop != NULL)
    { 
      LOGdL(DEBUG_STREAM,"calling receiver (%x) to stop...",
            stream->receiver.socketref.work_th.id.task);

      ret = stream->receiver.stop(&stream->receiver);
      if (ret)
	{
	  Error("DSI: stop receiver's failed: %s (%d)",
		l4env_errstr(ret),ret);
	  error = -DSI_ECOMPONENT;
	}
      LOGdL(DEBUG_STREAM,"receiver stop returned");
    }

  // close send component 
  LOGdL(DEBUG_STREAM,"calling sender (%x) to close...",
        stream->sender.socketref.work_th.id.task);

  ret = stream->sender.close(&stream->sender);
  if (ret)
    {    
      Error("DSI: close sender's socket failed: %s (%d)",
	    l4env_errstr(ret),ret);
      error = -DSI_ECOMPONENT;
    }
  LOGdL(DEBUG_STREAM,"sender close returned");

  // close receive component
  LOGdL(DEBUG_STREAM,"calling receiver (%x) to close...",
        stream->receiver.socketref.work_th.id.task);

  ret = stream->receiver.close(&stream->receiver);
  if (ret)
    {
      Error("DSI: close receiver's socket failed: %s (%d)",
	    l4env_errstr(ret),ret);
      error = -DSI_ECOMPONENT;
    }
  LOGdL(DEBUG_STREAM,"receiver close returned");

  // release stream descriptor 
  stream->flags = DSI_STREAM_UNUSED;

  // done 
  return error;
}
