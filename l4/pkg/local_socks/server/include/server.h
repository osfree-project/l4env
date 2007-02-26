/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/include/server.h
 * \brief  Header for socket server IDL glue layer and session management
 *         implementation.
 *
 * \date   16/01/2006
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOCAL_SOCKS_SERVER_H
#define __LOCAL_SOCKS_SERVER_H

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/sys/types.h>

/* ******************************************************************* */

void shutdown_session_of_client(l4_threadid_t client);
int  start_server(int events_support);

#endif /* __LOCAL_SOCKS_SERVER_H */
