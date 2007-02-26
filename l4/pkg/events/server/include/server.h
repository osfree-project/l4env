/*!
 * \file   events/server/include/server.h
 *
 * \brief  server API
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "l4/events/events.h"

/*! \brief Use a static array in bss as memory heap. The advantage is that
 * the rmgr detects conflicts for us. */
#define STATIC_MEMORY

/*! \brief start address for malloc pool */
#define MALLOC_POOL_ADDR  0x10000000
/*! \brief memory size used for malloc */
#define MALLOC_POOL_SIZE  0x00010000


/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles a register request.
 *
 * \param client	threadid of the client
 * \param event_ch	specifies the channel to register
 * \param priority	specefies the priority to register
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_register(l4_threadid_t *client, 
		l4events_ch_t event_ch, 
		l4events_pr_t priority);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles an unregister request.
 *
 * \param client	threadid of the client
 * \param event_ch	specifies the channel
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_unregister(l4_threadid_t *client, l4events_ch_t event_ch);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles an unregister all request.
 *
 * Calls server_unregister until all channels unregistered,
 *
 * \param client	threadid of the client
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_unregister_all(l4_threadid_t *client);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles a send request.
 *
 * Sends a long or a short event to waiting tasks or inserts the event in a
 * wait queue if some task registered but can not receive the event.
 *
 * \param client	threadid of the client
 * \param event_ch	specifies the channel
 * \param event		event to send
 * \param async 	a bit flag for blocking or non-blocking send
 *			\see WAIT and NO_WAIT
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_send_event(l4_threadid_t *client, l4events_ch_t event_ch,
		  const l4events_event_t *event, int async, int ack);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles a receive request.
 *
 * Tries to receive an event for the task. If this is not possible, make a note
 * that a thread is waiting for an event.
 *
 * \param client	threadid of the client
 * \retval event_ch	returns the received channel
 * \retval event	returns the received short event
 * \param ack		acknowledge after send
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_receive_event(l4_threadid_t *client, l4events_ch_t *event_ch,
			l4events_event_t *event, int ack);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles a give-ack request from the receiver.
 *
 * \param client	threadid of the client
 * \param event_nr	number of the event
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_give_ack(l4_threadid_t *client, l4events_nr_t event_nr);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles a get-ack request from the sender.
 *
 * \param client	threadid of the client
 * \param event_nr	number of the event
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_get_ack(l4_threadid_t *client, l4events_nr_t event_nr);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief This function is only used internally.
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
l4_uint8_t
server_handle_timeout(void);

/*****************************************************************************/
/*!\ingroup serverapi
 * \brief Handles a dump request.
 */
/*****************************************************************************/
void
server_dump(void);
