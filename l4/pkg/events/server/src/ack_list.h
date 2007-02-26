/*!
 * \file   events/server/src/ack_list.h
 *
 * \brief  This handles the ack-list for the sender.
 *
 * \date   19/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * This is the events server.
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "types.h"

#ifndef __L4EVENTS_ACK_LIST
#define __L4EVENTS_ACK_LIST

/* this is the list of sender waiting for notification */
ack_item_t*	ack_list = NULL;

#define link_item_into_ack_list(ack_list, curr_ack)			\
    curr_ack->next_item = ack_list;					\
    ack_list = curr_ack

#define unlink_item_from_ack_list(ack_list, curr_ack, prev_ack)		\
	if (prev_ack != NULL)						\
	  prev_ack->next_item = curr_ack->next_item;			\
	else								\
	  ack_list = curr_ack->next_item

#endif
