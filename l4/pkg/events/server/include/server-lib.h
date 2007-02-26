/*!
 * \file   events/server/include/server-lib.h
 *
 * \brief  server-library API
 *
 * \date   20/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * The server-library implements IPC handlung for communication from the
 * server to the client.
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "l4/events/events.h"

/*****************************************************************************/
/*!\ingroup serverlibapi
 * \brief Makes the reply for an receive of an event.
 *
 *
 * \param client	threadid of the client
 * \param event_ch	channel of the received event
 * \param event_nr	number of the received event
 * \param event		received event
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
int receive_event_reply(l4_threadid_t client, 
    			l4events_ch_t event_ch,
			l4events_nr_t event_nr,
			l4events_event_t event);

/*****************************************************************************/
/*!\ingroup serverlibapi
 * \brief Makes the reply to the sender after a send call.
 *
 * \param result	result value from send operation
 * \param client	threadid of the client
 * \param event_nr	unique number of this event
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
int send_event_reply(l4_uint16_t result, 
    			l4_threadid_t client, 
    			l4events_nr_t event_nr);

/*****************************************************************************/
/*!\ingroup serverlibapi
 * \brief Makes the reply to the sender after a get-ack call.
 *
 * \param client	threadid of the client
 * \param res		result value from get-ack call
 * \param channel_nr	number of the channel
 * \param event_nr	number of the event
 *
 * \return 		indicates a successful opration
 */
/*****************************************************************************/
int get_ack_reply(l4_threadid_t client,
    			l4_uint8_t res,
			l4events_ch_t channel_nr,
			l4events_nr_t event_nr);

/*****************************************************************************/
/*!\ingroup serverlibapi
 * \brief This function is used only internally.
 *
 * \param		threadid of the client
 * 
 * \return 		indicates a successful opration
 */ 
/*****************************************************************************/
int send_timeout(l4_threadid_t server);

/*****************************************************************************/
/*!\ingroup serverlibapi
 * \brief Makes the main server loop for client handling client requests.
 *
 */
/*****************************************************************************/
void server_loop(void* server_param);

