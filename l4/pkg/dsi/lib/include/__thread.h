/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__thread.h 
 * \brief  Thread handling prototypes. 
 *
 * \date   07/09/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___THREAD_H
#define _DSI___THREAD_H

/* includes */
#include <l4/sys/types.h>
#include <l4/thread/thread.h>
#include <l4/dsi/types.h>

/* prototypes */
int
dsi_create_sync_thread(dsi_socket_t * socket);
int
dsi_shutdown_sync_thread(dsi_socket_t * socket);

l4_threadid_t 
dsi_create_event_thread(l4thread_fn_t fn);

l4thread_t
dsi_create_select_thread(l4thread_fn_t fn, void * data, int tid, int nr);
void
dsi_shutdown_select_thread(l4thread_t thread);

#endif /* !_DSI___THREAD_H */
