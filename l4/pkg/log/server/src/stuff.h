/*!
 * \file	log/server/src/stuff.h
 * \brief       Log-Server, div. stuff like locking and thread creation.
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_SERVER_SRC_STUFF_H_
#define __LOG_SERVER_SRC_STUFF_H_

#include <l4/sys/types.h>


#define MAXTHREADS 3	/* max number of thread to be created by the
			 * logserver thread lib. */

/*!\brief Connection descriptor
 *
 * A connection is a unit, related to a client. A connection is bound to a
 * channel, multiple connections can use the same channel.
 *
 * A connection is unused if the channel id is 0.
 *
 * The protocol when writing data to a connection is as follows: One
 * packet can be buffered in the connection descriptor. Each time the
 * main-thread wants to store a packet in the connection descriptor, it
 * writes the address and size in the addr and size fields and increments
 * the written field. The flusher thread increments the flushed field, if the
 * buffer was successfully flushed. Therefore, the main-thread can determine
 * if the buffer is free by comparing the written and the flushed field.
 */
typedef struct{
    int  channel;	// channel id this connection sends on
    int	 written;	// nr of packets written into this connection
    int  flushed;	// nr of packets flushed on this connection
    void *addr;		// addr of the current packet
    int  size;		// size of the current packet
    l4_fpage_t fpage;   // the buffers flexpage
} bin_conn_t;


#define MAX_BIN_CONNS 64
extern bin_conn_t bin_conns[MAX_BIN_CONNS];

//! A structure containing the last received client-request
typedef struct{
    l4_fpage_t fpage;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t d0,d1, d2, d3, d4, d5, d6;
    l4_strdope_t string;
    l4_msgdope_t result;
} rcv_message_t;
extern rcv_message_t message;

extern unsigned buffer_head, buffer_tail, buffer_size;
extern char buffer_array[];
extern int flush_local, flush_serial, flush_to_net, flush_muxed;
extern int serial_esc;

extern int thread_create(void(*func)(void), l4_threadid_t *id,
			 const char*name);
int serial_flush(const char*addr, int size);

#endif
