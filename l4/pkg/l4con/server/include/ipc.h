/* $Id$ */
/**
 * \file	con/server/include/ipc.h
 * \brief	server ipc macros/functions
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _IPC_H
#define _IPC_H

/* L4 includes */
#include <l4/sys/ipc.h>

/* We assume that a client does l4_ipc_call for requests => snd to 0.
 * We want to leave the main loop from time to time      => rcv to 50ms */
#define REQUEST_TIMEOUT		L4_IPC_TIMEOUT(0,1,195,11,0,0)

/* We want to push an event to a client. 
 * The event handler may be busy handling the last event => snd to 50ms.
 * The handler needs some time to process the new event  => rcv to 50ms. */
#define EVENT_TIMEOUT		L4_IPC_TIMEOUT(195,11,195,11,0,0)

/**
 * Check if thread does exist.
 */
static inline int
thread_exists(l4_threadid_t thread)
{
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  int error;

  error = l4_ipc_receive(thread, 
			 L4_IPC_SHORT_MSG, &dw0, &dw1, 
			 L4_IPC_RECV_TIMEOUT_0, &result);

  return (error != L4_IPC_ENOT_EXISTENT);
}

#endif
