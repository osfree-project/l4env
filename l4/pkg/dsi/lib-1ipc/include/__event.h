/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__event.h
 * \brief  Event handling.
 *
 * \date   02/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___EVENT_H
#define _DSI___EVENT_H

/* includes */
#include <l4/sys/types.h>
#include <l4/dsi/dsi.h>

/* prototypes */
void
dsi_init_event_signalling(void);

int 
dsi_event_set(dsi_socketid_t socket_id, l4_uint32_t events);

int
dsi_event_reset(l4_threadid_t event_thread, dsi_socketid_t socket_id,
		l4_uint32_t events);

l4_int32_t
dsi_event_wait(l4_threadid_t event_thread, dsi_socketid_t socket_id,
	       l4_uint32_t events);

l4_threadid_t
dsi_get_event_thread_id(void);

#endif /* !_DSI___EVENT_H */
