/*!
 * \file   events/include/events.h
 *
 * \brief  constants and client-library API
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LIBEVENTS_H
#define __LIBEVENTS_H

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>
#include <l4/sys/types.h>


EXTERN_C_BEGIN

/* name used for registering at names server */
#define L4EVENTS_SERVER_NAME			"event_server"


/*****************************************************************************/
/*! \anchor return-makros
 * 
 *  constants for the warnings and errors returned
 *  by the events server and the library
 */
/*****************************************************************************/
/*! \brief Ok.
 *
 * The request was processed successful by the server.
 *
 */
#define L4EVENTS_OK				0

/*! \brief Task is already registered
 *
 * The task tries itself to register twice for the same channel. The request
 *  was ignored by the server.
 * \see l4events_register
 *
 */
#define L4EVENTS_WARNING_TASK_REGISTERED	0xf1

/*! \brief Task is not registered
 *
 * The task is not registered for this channel, but tries to unregister.
 * The request is ignoered by the server.
 * \see l4events_unregister
 *
 */
#define L4EVENTS_WARNING_TASK_NOT_REGISTERED	0xf2

/*! \brief Task lost pending events deleted during unregister
 *
 * The has itself unregistered for an channel. During the operation the server
 * removed some pending events from the wait-queue of this task. This may
 * cause serious information loss.
 * \see l4events_unregister
 *
 */
#define L4EVENTS_WARNING_EVENTS_DELETED		0xf3

/*! \brief There is no registered task for this channel
 *
 * The task has send an event. During the operation the server hasn't found
 * any registered task for this channel. The event was thrown away. This may
 * cause serious information loss.
 * \see l4events_send
 *
 */
#define L4EVENTS_WARNING_EVENTTYP_NOT_REGISTERED 0xf4

/*! \brief The task has no event to receive
 *
 * The thread tries to receive some event. During the operation the server
 * hasn't found any event suitable for this receive.
 * \see l4events_receive, l4events_receive_short
 *
 */
#define L4EVENTS_WARNING_NO_EVENT_TO_RECEIVE	0xf5

/*! \brief Some registered task has not received this event
 *
 * The thread tries to send some event. During the operation the server has 
 * found some registered tasks, but wasn't able send the event. The event
 * was appended to the wait queue for this task.
 *
 */
#define L4EVENTS_WARNING_EVENT_PENDING		0xf6

/*! \brief The event-number the task want to be acknowledged doesn't exist
 *
 * The task want to be acknowledged of the completion of sending an event,
 * but the corresponding event-number specified is invalid.
 *
 */
#define L4EVENTS_WARNING_INVALID_EVENTNR	0xf7

/*! \brief Internal server error occured
 *
 * The datastructures the server maintained are corrupted. If the problem
 * persists contact the maintainer.
 *
 */
#define L4EVENTS_ERROR_INTERNAL			0xf8

/*! \brief IPC error occured
 *
 * The reply sent by the server wasn't successful. The problem may be caused by
 * some errors in the client-library or the server-library. Contact the
 * maintainer.
 *
 */
#define L4EVENTS_ERROR_IPC			0xf9

/*! \brief IPC timeout occured
 *
 * The client-library detected an IPC timeout. This could normally only happen
 * during a receive operation with timeout set. The error should be handled by
 * client.
 * \see l4events_receive, l4events_receive_short
 *
 */
#define L4EVENTS_ERROR_TIMEOUT			0xfa

/*! \brief Unknown command was sent
 *
 * The server-library can't interpret the command sent. This error normally
  shouldn't happen with using client-library for making requests to the server.
 *
 */
#define L4EVENTS_ERROR_INVALID_COMMAND		0xfb

/*! \brief Undefined error occured
 */
#define L4EVENTS_ERROR_OTHER			0xfc

/*! \brief another server replied to an open wait call.
 */
#define L4EVENTS_RECV_OTHER			0xfd

/*! \brief thread does not exists or we are not allowed to communicate
 */
#define L4EVENTS_ERROR_NOT_EXISTS               0xfe

#define L4EVENTS_ERRSTRINGS_DEFINE					\
  { L4EVENTS_WARNING_TASK_REGISTERED,					\
    "EVENTS: Task is already registered" },				\
  { L4EVENTS_WARNING_TASK_NOT_REGISTERED,				\
    "EVENTS: Task is not registered" } ,				\
  { L4EVENTS_WARNING_EVENTS_DELETED,					\
    "EVENTS: Task lost pending events deleted during unregister" },	\
  { L4EVENTS_WARNING_EVENTTYP_NOT_REGISTERED,				\
    "EVENTS: There is no registered task for this channel" },		\
  { L4EVENTS_WARNING_NO_EVENT_TO_RECEIVE,				\
    "EVENTS: No event to receive" },					\
  { L4EVENTS_WARNING_EVENT_PENDING,					\
    "EVENTS: Some registered task has not received this event" },	\
  { L4EVENTS_WARNING_INVALID_EVENTNR,					\
    "EVENTS: Acknowledge: Event-number doesn't exist" },		\
  { L4EVENTS_ERROR_INTERNAL,						\
    "EVENTS: Internal server error occured" },				\
  { L4EVENTS_ERROR_IPC,							\
    "EVENTS: IPC error occured" },					\
  { L4EVENTS_ERROR_TIMEOUT,						\
    "EVENTS: IPC timeout occured" },					\
  { L4EVENTS_ERROR_INVALID_COMMAND,					\
    "EVENTS: Unknown command was sent " },				\
  { L4EVENTS_ERROR_OTHER,						\
    "EVENTS: Undefined error occured" },				\
  { L4EVENTS_RECV_OTHER,						\
    "EVENTS: Unknown server replied to open wait" }

/****************************************************************************
 * \anchor receive and send options
 * 
 ****************************************************************************/

/*!
 * \brief non-blocking send
 *
 * The sender is not blocked while the server tries to send
 * the event to all registered tasks. The default is that
 * the sender is blocked while the server tries to send
 * the event to all registered tasks.
 *
 * \see l4events_send
 */
#define L4EVENTS_ASYNC		1

/*!
 *\brief acknowledge after sending
 *
 * The sender can get acknowledged after all registered
 * tasks for the channel have received the event.
 * The acknowledge can be requested with ::l4events_get_ack.
 * The default is to get no acknowledge.
 *
 * \see l4events_send and \see l4events_get_ack
 */
#define L4EVENTS_SEND_ACK	2

/*!
 *\brief give acknowledge after receive
 *
 * The receiver can give acknowledged after processing the
 * received event.
 * The acknowledge can be given with ::l4events_give_ack or
 * combined with ::l4events_give_ack_and_receive.
 * The default is to get no acknowledge.
 *
 * \see l4events_receive and \see l4events_give_ack
 */
#define L4EVENTS_RECV_ACK	4

/*!
 *\brief give acknowledge and receive
 *
 * Use the API function ::l4events_give_ack_and_receive.
 *
 */
#define L4EVENTS_GIVE_ACK	8

/*!
 *\brief receive a short event
 *
 * The receiver wants to receive a short event.
 * The size of the event depends on the implementation.
 */
#define L4EVENTS_RECV_SHORT	16

/*! \brief definition of a channel */
typedef l4_uint16_t l4events_ch_t;

/*! \brief event_ch 0 is reserved */
#define L4EVENTS_NO_CHANNEL	0

/*! \brief event_ch for task exit */
#define L4EVENTS_EXIT_CHANNEL	1

/*! \brief definition of the maximum length of an event */
#define L4EVENTS_MAX_BUFFER_SIZE	60

/*! \brief maximum priority */
#define L4EVENTS_MAX_PRIORITY	15

/*****************************************************************************/
/*!\brief definition of messge (event)
 */
/*****************************************************************************/
typedef struct {
  int	len;				//!< length of the message
  char	str[L4EVENTS_MAX_BUFFER_SIZE];	//!< message-buffer
} l4events_event_t;

/*! \brief definition of an event-nr */
typedef l4_umword_t l4events_nr_t;

typedef l4_int8_t l4events_pr_t;

/*! \brief event-nr 0 is reserved */
#define L4EVENTS_NO_NR	0

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Initializes the event server.
 *
 * \return 1 if event server was found; 0 otherwise
 */
/*****************************************************************************/
L4_CV int
l4events_init(void);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Registers a task with its task id for an channel.
 *
 *
 * \param event_ch	specifies the channel
 * \param priority	specifies the priority
 * 			(0 is lowest ... 15 is highest)
 *			If bit 7 is set the client is assumed as untrusted,
 *			that is if the client is not ready to receive an
 *			event immediately, the event is assumed as successfully
 *			delivered.
 *
 * \return 		indicates a successful opration, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_register(const l4events_ch_t event_ch, 
     		  const l4events_pr_t priority);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Unregisters a task for an channel.
 *
 * \param event_ch specifies the channel
 *
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_unregister(const l4events_ch_t event_ch);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Unregisters a task for all previous registered channels.
 *
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_unregister_all(void);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Sends an event to all registered tasks.
 *
 * \param event_ch 	specifies the channel
 * \param event 	event to send
 * \param opt 		options for send
 *			\see L4EVENTS_ASYNC and L4EVENTS_ACK
 *
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_send(const l4events_ch_t event_ch, 
    	            l4events_event_t *event,
	            l4events_nr_t *event_nr,
	      const int opt);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Receives an event.
 *
 * \param event_ch	channel to receive (specify L4EVENTS_NO_ID for all)
 * \retval event_ch	returns the received channel
 * \retval event	returns the received event
 * \retval event_nr	returns the unique event-number of the event
 * \param  timeout 	specifies the IPC timeout
 * \param  opt		options for receive
 * 			\see L4EVENTS_RECV_ACK
 *  
 * 
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_receive(l4events_ch_t *event_ch, 
    		 l4events_event_t *event,
		 l4events_nr_t *event_nr,
		 l4_timeout_t timeout,
		 const int opt);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Acknowledges and receives an event.
 *
 * \param  event_ch	channel to receive (specify L4EVENTS_NO_ID for all)
 * \retval event_ch	returns the received channel
 * \retval event	returns the received short event
 * \param  event_nr	event-number for the event to acknowledge
 * \retval event_nr	returns the unique event-number of the event
 * \param  timeout	specifies the IPC timeout
 * \param  opt		options for receive
 * 			\see L4EVENTS_RECV_ACK
 *
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_give_ack_and_receive(l4events_ch_t *event_ch,
				l4events_event_t *event,
				l4events_nr_t *event_nr,
				l4_timeout_t timeout,
				const int opt);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Dumps some information about the event server.
 *
 * This displays the current state of the data structures.
 * If the option debug_malloc is switched on, then also memory usage is shown.
 *
 * \return 		indicates a successful opration, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV int
l4events_dump(void);

/*!\brief callback function type definition
 *
 * \retval 1	received channel
 * \retval 2	received event
 *
 */
typedef L4_CV void (*l4events_recv_function_t)(l4events_ch_t, l4events_event_t *);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Starts a thread that waits for an event
 *
 * \param threadno the threadno of the wait-thread
 * \param event_ch the channel to be waited for
 * \param function callback function which is responsible for processing the
 *        received event
 */
/*****************************************************************************/
L4_CV void
l4events_wait(int threadno, l4events_ch_t event_ch,
	      l4events_recv_function_t function);


/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Tries to get acknowledge after sending an event.
 *
 * \param event_nr	event_nr for which to get acknowledge
 * \param timeout	specifies the IPC timeout
 *
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_get_ack(l4events_nr_t *event_nr, l4_timeout_t timeout);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Tries to get acknowledge after sending an event with an open wait.
 *
 * \param event_nr	event_nr for whicch  to get acknowledge
 * \param timeout	specifies the IPC timeout
 *
 * \retval sender	contains id of sender if timeout than L4_INVALID_ID
 * \retval w1		value of dword1 if sender was not events
 * \retval w2		value of dword2 if sender was not events
 * 
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_get_ack_open(l4events_nr_t *event_nr, l4_threadid_t *id,
    		      l4_umword_t *w1, l4_umword_t *w2, l4_timeout_t timeout);

/*****************************************************************************/
/*!\ingroup clientlibapi
 * \brief Tries to set acknowledge after receiving an event.
 *
 * \param event_ch	event_nr for whicch  to give acknowledge
 * 			after receiving
 *
 * \return		indicates a successful operation, please see
 *			\ref return-makros for possible return values
 */
/*****************************************************************************/
L4_CV long
l4events_give_ack(l4events_nr_t event_nr);

asm(".globl l4events_err_strings_sym; .type l4events_err_strings_sym,%object");

EXTERN_C_END

#endif
