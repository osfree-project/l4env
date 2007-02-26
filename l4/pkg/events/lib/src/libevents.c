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

#include "l4/events/events.h"
#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include "lib.h"
#include "message.h"


/*!\brief wraps the l4events_register call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_register(const l4events_ch_t event_ch,
    		  const l4events_pr_t priority)
{
  l4_umword_t w1 = create_control_word_for_call(REGISTER_EVENT, 0, event_ch, 
      						L4EVENTS_NO_NR);
  l4_umword_t w2 = priority; /* lowest 4 bits */

  return l4events_send_short_message(&w1, &w2, L4_IPC_NEVER);
};

/*!\brief wraps the l4events_unregister call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_unregister(const l4events_ch_t event_ch)
{
  l4_umword_t w1 = create_control_word_for_call(UNREGISTER_EVENT, 0, event_ch, 
						L4EVENTS_NO_NR);
  l4_umword_t w2 = 0;

  return l4events_send_short_message(&w1, &w2, L4_IPC_NEVER);
};

/*!\brief wraps the l4events_unregister_all call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_unregister_all(void)
{
  l4_umword_t w1 = create_control_word_for_call(UNREGISTER_ALL_EVENTS, 0, 
      						L4EVENTS_NO_CHANNEL, 
						L4EVENTS_NO_NR);
  l4_umword_t w2 = 0;

  return l4events_send_short_message(&w1, &w2, L4_IPC_NEVER);
};

/*!\brief wraps the l4events_send call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_send(const l4events_ch_t event_ch, l4events_event_t *evt,
    	      l4events_nr_t *event_nr, const int opt)
{
  message_t   msg;
  l4_umword_t w1;
  long        res;
  l4_uint8_t  cmd;

  init_message(&msg, evt);

  /* code options in command */
  switch (opt)
  {
    case L4EVENTS_ASYNC:
      cmd = ASYNC_SEND_EVENT;
      break;
    case L4EVENTS_SEND_ACK:
      cmd = ACK_SEND_EVENT;
      break;
    case L4EVENTS_ASYNC|L4EVENTS_SEND_ACK:
      cmd = ASYNC_ACK_SEND_EVENT;
      break;
    default:
      cmd = SEND_EVENT;
      break;
  };
  
  msg.cr = create_control_word_for_call(cmd, evt->len, event_ch, 
      					L4EVENTS_NO_NR);

  if (evt->len <= SHORT_BUFFER_SIZE)
  {
    w1 = get_first_word(evt);
    res = l4events_send_short_message(&msg.cr, &w1, L4_IPC_NEVER);
    *event_nr = get_event_nr(msg.cr);
  }
  else
  {
    res = l4events_send_message(&msg, L4_IPC_NEVER);
    *event_nr = get_event_nr(msg.cr);
  }

  return res;
};

/*!\brief wraps the l4events_receive call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_receive(l4events_ch_t *id, 
    		 l4events_event_t *evt,
		 l4events_nr_t *nr,
		 l4_timeout_t timeout,
		 const int opt)
{
  message_t   msg;
  l4_umword_t w1, w2;
  long        res;
  l4_uint8_t  cmd;
  l4_uint8_t  copt;

  init_message(&msg, evt);

  copt = opt & (L4EVENTS_RECV_ACK | L4EVENTS_GIVE_ACK);
    
  switch (copt)
    {
    case L4EVENTS_RECV_ACK:
      cmd = ACK_RECEIVE_EVENT;
      break;
    case L4EVENTS_GIVE_ACK:
      cmd = GIVE_ACK_AND_RECEIVE_EVENT;
      break;
    case L4EVENTS_RECV_ACK | L4EVENTS_GIVE_ACK:
      cmd = GIVE_ACK_AND_ACK_RECEIVE_EVENT;
      break;
    default:
      cmd = RECEIVE_EVENT;
      break;
    }
  
  if (opt & L4EVENTS_RECV_SHORT)
    {
      w1  = create_control_word_for_call(cmd, SHORT_BUFFER_SIZE, *id, *nr);
      w2  = *(l4_umword_t*)evt->str;
      res = l4events_send_short_message(&w1, &w2, timeout);
      *id = get_event_ch(w1);
      *nr = get_event_nr(w1);
      evt->len = get_len(w1);
      set_first_word(w2, evt);
    }
  else
    {
      w1  = create_control_word_for_call(cmd, L4EVENTS_MAX_BUFFER_SIZE, 
	                                 *id, *nr);

      res = l4events_send_recv_message(w1, &msg, timeout);
      *id = get_event_ch(msg.cr);
      *nr = get_event_nr(msg.cr);
      evt->len = get_len(msg.cr);

      if (evt->len <= SHORT_BUFFER_SIZE)
    	set_first_word(msg.str.w1, evt);
    }

  return res;
}


/*!\brief wraps the l4events_give_ack_and_receive call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_give_ack_and_receive(
    		l4events_ch_t *id, 
    		l4events_event_t *evt,
		l4events_nr_t *nr,
		l4_timeout_t timeout,
		const int opt)
{
  return l4events_receive(id, evt, nr, timeout, opt | L4EVENTS_GIVE_ACK);
}

/*!\brief wraps the l4events_get_ack call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_get_ack(l4events_nr_t *event_nr, l4_timeout_t timeout)
{
  l4_umword_t w1 = create_control_word_for_call(GET_ACK, SHORT_BUFFER_SIZE, 
						0, *event_nr);
  l4_umword_t w2 = 0;
  long        res;

  res = l4events_send_short_message(&w1, &w2, timeout);

  *event_nr = get_event_nr(w1);

  return res;
}

/*!\brief wraps the l4events_get_ack_open call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
long
l4events_get_ack_open(l4events_nr_t *event_nr, l4_threadid_t *id,
   		      l4_umword_t *w1, l4_umword_t *w2, l4_timeout_t timeout)
{
  long res;

  *w1 = create_control_word_for_call(GET_ACK, SHORT_BUFFER_SIZE, 0, *event_nr);
  *w2 = 0;

  res = l4events_send_short_open_message(id, w1, w2, timeout);

  if (res == L4EVENTS_OK)
    {
      *event_nr = get_event_nr(*w1);
      *w1 = 0;
      *w2 = 0;
    }
  else
    *event_nr = L4EVENTS_NO_NR;

  return res;
}

long
l4events_give_ack(l4events_nr_t event_nr)
{
  l4_umword_t w1 = create_control_word_for_call(GIVE_ACK, SHORT_BUFFER_SIZE, 
						0, event_nr);
  l4_umword_t w2 = 0;
  long        res;

  res = l4events_send_short_message(&w1, &w2, L4_IPC_NEVER);

  return res;
}

/*!\brief wraps the l4events_dump call
 * \ingroup internal
 *
 * This function calls the lower level IPC function.
 */
int
l4events_dump(void)
{
  l4_umword_t w1;
  l4_umword_t w2;
  long        res;

  w1 = create_control_word_for_call(SERVER_DUMP, 0, 
      					L4EVENTS_NO_CHANNEL, 
					L4EVENTS_NO_NR);
  w2 = 0;

  res = l4events_send_short_message(&w1, &w2, L4_IPC_NEVER);

  return res;
};

