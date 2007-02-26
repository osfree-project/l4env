/*!
 * \file   events/lib/src/libevents.c
 *
 * \brief  IPC stub client library
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>

#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

#include "lib.h"
#include "message.h"


l4_threadid_t l4events_server = L4_INVALID_ID;


/*!\brief makes pure long IPC call
 * \ingroup internal
 *
 * This function sends long IPC and waits for long IPC reply.
 */
long
l4events_send_message(message_t* msg, l4_timeout_t timeout)
{
  int ipc_error;
  l4_msgdope_t	result;

  if (EXPECT_FALSE(l4_is_invalid_id(l4events_server)))
    if (!l4events_init())
      return -L4EVENTS_ERROR_OTHER;

  ipc_error = l4_ipc_call(l4events_server,
			  msg, msg->cr, msg->str.w1,
			  msg, &msg->cr, &msg->str.w1,
			  timeout, &result);

  if (!ipc_error)
    return -get_res(msg->cr);

  if (ipc_error == L4_IPC_RETIMEOUT )
    return -L4EVENTS_ERROR_TIMEOUT;

  return -L4EVENTS_ERROR_IPC;
}

/*!\brief makes pure short IPC call
 * \ingroup internal
 *
 * This function sends short IPC and waits for short IPC reply.
 */
long
l4events_send_short_message(l4_umword_t *w1, l4_umword_t *w2,
				l4_timeout_t timeout)
{
  int ipc_error;

  l4_msgdope_t	result;

  if (EXPECT_FALSE(l4_is_invalid_id(l4events_server)))
    if (!l4events_init())
      return -L4EVENTS_ERROR_OTHER;

  ipc_error = l4_ipc_call(l4events_server,
			  L4_IPC_SHORT_MSG, *w1, *w2,
	  		  L4_IPC_SHORT_MSG, w1, w2,
			  timeout, &result);

  if (!ipc_error)
    return -get_res(*w1);

  if (ipc_error == L4_IPC_RETIMEOUT)
    return -L4EVENTS_ERROR_TIMEOUT;

  return -L4EVENTS_ERROR_IPC;
}

/*!\brief makes pure short IPC call
 * \ingroup internal
 *
 * This function sends short IPC and makes an open wait 
 * for short IPC reply.
 */
long
l4events_send_short_open_message(l4_threadid_t *id,
    				 l4_umword_t *w1, l4_umword_t *w2,
			  	 l4_timeout_t timeout)
{
  int ipc_error;

  l4_msgdope_t	result;

  if (EXPECT_FALSE(l4_is_invalid_id(l4events_server)))
    if (!l4events_init())
      return -L4EVENTS_ERROR_OTHER;

  ipc_error = l4_ipc_reply_and_wait(l4events_server,
				    L4_IPC_SHORT_MSG,
				    *w1, *w2,
				    id,
				    L4_IPC_SHORT_MSG,
				    w1, w2,
				    timeout, &result);

  if (!ipc_error)
    {
      return (l4_thread_equal(*id, l4events_server)) 
	? -get_res(*w1) 
	: -L4EVENTS_RECV_OTHER;
    }

  *id = L4_INVALID_ID;

  if (ipc_error == L4_IPC_RETIMEOUT || ipc_error == L4_IPC_SETIMEOUT)
    return -L4EVENTS_ERROR_TIMEOUT;

  return -L4EVENTS_ERROR_IPC;
}

/*!\brief makes mixed short ans long IPC call
 * \ingroup internal
 *
 * This function sends short IPC and waits for long IPC reply.
 */
long
l4events_send_recv_message(l4_umword_t w1, message_t* msg, l4_timeout_t timeout)
{
  int ipc_error;
  l4_msgdope_t	result;

  if (EXPECT_FALSE(l4_is_invalid_id(l4events_server)))
    if (!l4events_init())
      return -L4EVENTS_ERROR_OTHER;

  ipc_error = l4_ipc_call(l4events_server,
			  L4_IPC_SHORT_MSG, w1, 0,
			  msg,&msg->cr, &msg->str.w1,
			  timeout, &result);

  if (!ipc_error)
    return -get_res(msg->cr);

  if (ipc_error == L4_IPC_RETIMEOUT)
    return -L4EVENTS_ERROR_TIMEOUT;

  return -L4EVENTS_ERROR_IPC;
}
