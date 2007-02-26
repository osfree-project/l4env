/*!
 * \file   events/server/src/lib.c
 *
 * \brief  IPC stub library for the server
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/macros.h>

#include "globals.h"
#include "lib.h"
#include "server.h"
#include "server-lib.h"

/* for readability :-) */
#define false 0
#define true 1

/*!\brief replies to receive_event call
 * \ingroup internal
 *
 * This function makes depending on the event-length
 * a short or a long IPC.
 */
int
receive_event_reply(l4_threadid_t client, 
    			l4events_ch_t event_ch,
			l4events_nr_t event_nr,
			l4events_event_t event)
{
  message_t	msg;
  l4_msgdope_t 	result;
  l4_umword_t	first_word;

  init_message(&msg, &event);

  msg.cr = create_control_word_for_reply(L4EVENTS_OK,event.len,event_ch, event_nr);

  if (event.len <= SHORT_BUFFER_SIZE)
  {
    first_word = get_first_word(&event);

    return l4_ipc_send(client, L4_IPC_SHORT_MSG,
  			msg.cr, first_word,
			L4_IPC_SEND_TIMEOUT_0,
			&result);
  }
  else
  {
    return l4_ipc_send(client, &msg,
  			msg.cr, msg.str.w1,
			L4_IPC_SEND_TIMEOUT_0,
			&result);
  }
}

/*!\brief replies to send_event call
 * \ingroup internal
 *
 * This function makes a short IPC call. The server replies to
 * the sender after sending the event.
 */
int
send_event_reply(l4_uint16_t result, l4_threadid_t client, 
    		 l4events_nr_t event_nr)
{
  l4_umword_t	w1, w2;
  l4_msgdope_t	res;

  w1 = create_control_word_for_reply(result, 0, 0, event_nr);
  w2 = 0;

  return l4_ipc_send(client, L4_IPC_SHORT_MSG,
			w1, w2,
			L4_IPC_SEND_TIMEOUT_0,
			&res);
}

/*!\brief replies to get_ack call
 * \ingroup internal
 *
 * This function makes a short IPC call. The server replies the ack
 * to the sender.
 */
int
get_ack_reply(l4_threadid_t client,
    		l4_uint8_t result,
    		l4events_ch_t event_ch,
		l4events_nr_t event_nr)
{
  l4_umword_t	w1, w2;
  l4_msgdope_t	res;
  int ret;

  w1 = create_control_word_for_reply(result, 0, event_ch, event_nr);
  w2 = 0;

  ret = l4_ipc_send(client, L4_IPC_SHORT_MSG,
			w1, w2,
			L4_IPC_SEND_TIMEOUT_0,
			&res);

  return ret;
}

/*!\brief makes the server loop
 * \ingroup internal
 *
 * This function implements the server loop.
 */
void
server_loop(void* server_param)
{
  /* variables for IPC handling */
  message_t	message;
  l4_msgdope_t	result;
  l4_threadid_t	client;
  unsigned	error;
  int		do_reply;
  l4_umword_t	w1, w2;

  /* variables for unwrapping information */
  l4events_ch_t 	event_ch;
  l4events_event_t	event;
  l4events_nr_t		event_nr;
  l4events_pr_t		priority = 0; /* set to value */
  int			event_cmd;
  int			event_result = L4EVENTS_OK;
  int			send_async;
  int			send_ack;
  int			recv_ack;
  extern void *timeout_first_event;

  /* init message structure variable */
  init_message(&message, &event);

  for (;;)
    {
      /* Resume not later than 10ms if we have entries in the timeout queue.
       * Wait forever if there are no entries in the timeout queue */
      error = l4_ipc_wait(&client, &message,
			  &message.cr, &message.str.w1,
			  timeout_first_event ? SERVER_TIMEOUT : L4_IPC_NEVER,
			  &result);
      for (;;)
	{
	  if (error == L4_IPC_RETIMEOUT)
	    {
	      event_result = server_handle_timeout();
	      error = 0;
	      break;
	    }

    	  if (error)
	    break;

	  /* extract the wrapped information from sent message */
	  event_cmd = get_cmd(message.cr);
	  event.len = get_len(message.cr);
	  event_ch  = get_event_ch(message.cr);
	  event_nr  = get_event_nr(message.cr);
	  priority  = message.str.w1;

	  /* special handling for receiving short messages */
	  if (event.len < 5) 
	    /* XXX use symbolic name SHORT_BUFFER_SIZE from lib !!! */
	    {
	      set_first_word(message.str.w1, &event);
	    }

	  /* decode flags from command */
	  send_async = (event_cmd == ASYNC_SEND_EVENT ||
		        event_cmd == ASYNC_ACK_SEND_EVENT);

	  send_ack   = (event_cmd == ACK_SEND_EVENT ||
			event_cmd == ASYNC_ACK_SEND_EVENT);

	  recv_ack   = (event_cmd == ACK_RECEIVE_EVENT ||
	 		event_cmd == GIVE_ACK_AND_ACK_RECEIVE_EVENT);

	  do_reply = true;

    	  /* depending on the command we switch to a server function */
	  switch(event_cmd)
	    {
	    case REGISTER_EVENT:
	      event_result = server_register(&client, event_ch, priority);
	      break;

	    case UNREGISTER_EVENT:
	      event_result = server_unregister(&client, event_ch);
	      break;

	    case UNREGISTER_ALL_EVENTS:
	      event_result = server_unregister_all(&client);
	      break;

	    case SEND_EVENT:
	    case ASYNC_SEND_EVENT:
	    case ACK_SEND_EVENT:
	    case ASYNC_ACK_SEND_EVENT:
	      event_result = server_send_event(&client, event_ch, &event, 
	      				        send_async, send_ack);
	      do_reply = false;
	      break;

	    case RECEIVE_EVENT:
	    case ACK_RECEIVE_EVENT:
	      event_result = server_receive_event(&client, &event_ch, 
	      					  &event, recv_ack);
	      do_reply = false;
	      break;

	    case GIVE_ACK_AND_RECEIVE_EVENT:
	    case GIVE_ACK_AND_ACK_RECEIVE_EVENT:
	      event_result = server_give_ack(&client, event_nr);	
	      event_result = server_receive_event(&client, &event_ch, 
	      					  &event, recv_ack);
	      do_reply = false;
	      break;
	  
	    case GIVE_ACK:
	      event_result = server_give_ack(&client, event_nr);
	      break;
	  
	    case GET_ACK:
	      event_result = server_get_ack(&client, event_nr); 
	      do_reply = false;
	      break;

	    case SERVER_DUMP:
	      server_dump();
	      break;

	    default:
	      event_result = L4EVENTS_ERROR_INVALID_COMMAND;
	      break;
	    };

	  if (!do_reply)
	    break;

	  w1 = create_control_word_for_reply(event_result, 0, 0, 0);
	  w2 = 0;

	  error = l4_ipc_reply_and_wait(client,
					L4_IPC_SHORT_MSG, w1, w2,
					&client, 
					&message, &message.cr, &message.str.w1,
					SERVER_TIMEOUT, &result);
	}
      if (error)
	LOG("IPC error: client "l4util_idfmt" error %02x",
	    l4util_idstr(client), error);
    }
}
