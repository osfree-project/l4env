/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__socket.h
 * \brief  Socket definitions / prototypes
 *
 * \date   07/ß9/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___SOCKET_H
#define _DSI___SOCKET_H

/* library includes */
#include <l4/dsi/types.h>

/* macros to check socket flags */
#define IS_SEND_SOCKET(s)          (s->flags & DSI_SOCKET_SEND)
#define IS_RECEIVE_SOCKET(s)       (s->flags & DSI_SOCKET_RECEIVE)
#define SOCKET_BLOCK(s)            (s->flags & DSI_SOCKET_BLOCK)
#define SOCKET_SYNC_CALLBACK(s)    (s->flags & DSI_SOCKET_SYNC_CALLBACK)
#define SOCKET_RELEASE_CALLBACK(s) (s->flags & DSI_SOCKET_RELEASE_CALLBACK)

/* prototypes */
void
dsi_init_sockets(void);

int
dsi_is_valid_socket(dsi_socket_t * socket);

#endif /* !_DSI___SOCKET_H */
