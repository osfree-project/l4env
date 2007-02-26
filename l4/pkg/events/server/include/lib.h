/*!
 * \file   events/server/include/lib.h
 *
 * \brief  definitions and utility functions for IPC library
 *
 * \date   20/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#include "l4/events/events.h"

/* function codes */
#define	REGISTER_EVENT		1 /*!< \ingroup privatelibapi register_event call */
#define	UNREGISTER_EVENT	2 /*!< \ingroup privatelibapi unregister_event call */
#define UNREGISTER_ALL_EVENTS	3 /*!< \ingroup privatelibapi not implemented */
#define SEND_EVENT		4 /*!< \ingroup privatelibapi send_event with wait call */
#define ASYNC_SEND_EVENT	5 /*!< \ingroup privatelibapi send_event with no wait call */
#define ACK_SEND_EVENT		6 /*!< \ingroup privatelibapi */
#define ASYNC_ACK_SEND_EVENT	7 /*!< \ingroup privatelibapi */

#define RECEIVE_EVENT		8 /*!< \ingroup privatelibapi receive_event call */
#define ACK_RECEIVE_EVENT	9 /*!< \ingroup privatelibapi receive_event call with acknowledge */
#define GET_ACK			10 /*!< \ingroup privatelibapi get notification call */
#define GIVE_ACK		11 /*!< \ingroup privatelibapi give notification after receiving */
#define GIVE_ACK_AND_RECEIVE_EVENT \
				12 /*!<ingroup privatelibapi */

#define GIVE_ACK_AND_ACK_RECEIVE_EVENT \
				13 /*!< \ingroup privatelibapi */

#define TIMEOUT			14 /*!< \ingroup privatelibapi */

#define SERVER_DUMP		15 /*!< \ingroup privatelibapi dump call */

/*! \ingroup privatelibapi
 *  \brief this value is valid for l4v2 target-architecture */
#define SHORT_BUFFER_SIZE 4


/*! \ingroup privatelibapi
 *  \brief defines the message typ for requests send to the server */
typedef struct {
  l4_fpage_t	fpage;
  l4_msgdope_t	size_dope;
  l4_msgdope_t	send_dope;

  /*! control word includes: command, result, event_ch, string length */
  l4_umword_t	cr;

  /*! \brief union for special handling the first word */
  union
  {
    l4_umword_t		w1;		//!< the first word
    l4_strdope_t	string;		//!< contains the event
  } str;
} message_t;

void copy_evt_to_msg(const l4events_event_t evt, message_t *msg);
void copy_msg_to_evt(const message_t msg, l4events_event_t *evt);

/*!\ingroup privatelibapi
 * \brief initializes the message structure for long IPC */
static inline void init_message(message_t* msg, l4events_event_t* evt)
{
  msg->fpage.fpage = 0;
  msg->size_dope = L4_IPC_DOPE(1,1);
  msg->send_dope = L4_IPC_DOPE(1,1);

  msg->cr = 0;

  msg->str.string.snd_str = (l4_umword_t) evt->str;
  msg->str.string.rcv_str = (l4_umword_t) evt->str;
  msg->str.string.snd_size = L4EVENTS_MAX_BUFFER_SIZE;
  msg->str.string.rcv_size = L4EVENTS_MAX_BUFFER_SIZE;
};

/*!\ingroup privatelibapi
 * \brief utility function for short IPC handling */
static inline l4_umword_t get_first_word(l4events_event_t *event)
{
  return *(l4_umword_t*)event->str;
}

/*!\ingroup privatelibapi
 * \brief utility function for short IPC handling */
static inline void set_first_word(l4_umword_t word, l4events_event_t *event)
{
  *(l4_umword_t*)event->str = word;
}

/*!\ingroup privatelibapi
 * \brief assembles the control word from function code, event length,
 *         event_ch */
#define create_control_word_for_call(cmd, len, event_ch, event_nr) \
	((((((cmd & 0xf) << 8) | \
	     (len & 0xff)) << 8) | \
	     (event_ch & 0xff)) << 12) | \
	     (event_nr & 0xfff)

/*!\ingroup privatelibapi
 * \brief assembles the control word from result value, event length,
 *        event_ch */
// XXX caution we lost the highest 4 bit of result value
#define create_control_word_for_reply(res, len, event_ch, event_nr) \
	((((((res & 0xf) << 8) | \
	     (len & 0xff)) << 8) | \
             (event_ch & 0xff)) << 12) | \
	     (event_nr & 0xfff)

/*!\ingroup privatelibapi
 * \brief returns the function code */
#define get_cmd(word) \
	((word >> 28) & 0xf)

/*!\ingroup privatelibapi
 * \brief returns the result value */
// XXX fix result value becaue we only transfer the lower 4 bit
#define get_res(word) \
	(((word >> 28) & 0xf) ? ((word >> 28) & 0xf) | 0xf0 : 0)

/*!\ingroup privatelibapi
 * \brief returns the length of the message buffer */
#define get_len(word) \
	((word >> 20) & 0xff)

/*!\ingroup privatelibapi
 * \brief returns the event_ch */
#define get_event_ch(word) \
	((word >> 12) & 0xff)

/*!\ingroup privatelibapi
 * \brief returns the event_nr of the event */
#define get_event_nr(word) \
  	(word & 0xfff)


