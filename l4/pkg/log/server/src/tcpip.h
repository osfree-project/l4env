/*!
 * \file   log/server/src/tcpip.h
 * \brief  
 *
 * \date   03/14/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_SERVER_SRC_TCPIP_H_
#define __LOG_SERVER_SRC_TCPIP_H_

#include "config.h"

#if CONFIG_USE_TCPIP

#include <oskit/net/socket.h>

extern char *ip_addr, *netmask;
extern oskit_socket_t*client_socket;

/* variable to hold the socket creator function */
extern oskit_socket_factory_t *socket_create;
extern int net_init(int argc, char **argv);
extern int net_wait_for_client(void);
extern int net_receive_check(void);
extern int net_flush_buffer(const char*addr, int size);
#endif

#endif
