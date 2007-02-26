/*!
 * \file	log_ore/muxed.h
 * \brief   binary data stuff, adapted from log/server/src/muxed.h
 *          and log/server/src/stuff.h
 *
 * \date	12/21/2005
 * \author	Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_SERVER_SRC_STUFF_H_
#define __LOG_SERVER_SRC_STUFF_H_

#include <l4/sys/types.h>

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


/* define protocol version tag */
#define LOG_PROTOCOL_VERSION  0x01

/* Type constants */
#define LOG_TYPE_UNKNOWN  1
#define LOG_TYPE_DATA     2
#define LOG_TYPE_LOG      3

/* Flags - none used at the moment */
#define LOG_FLAG_NONE     0

#define MAX_BIN_CONNS 64
extern bin_conn_t bin_conns[MAX_BIN_CONNS];

//! A structure containing the last received client-request
typedef struct{
    l4_fpage_t fpage;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t dw0,dw1, dw2, dw3, dw4, dw5, dw6;
    l4_strdope_t string;
    l4_msgdope_t result;
} rcv_message_t;

extern rcv_message_t message;

/* the header */
struct muxed_header {
	unsigned char  version;
	unsigned char  channel;
	unsigned char  type;
	unsigned char  flags;
	unsigned int   data_length;
};

l4_fpage_t channel_get_next_fpage(void);
int channel_open(void);
int channel_write(void);
int channel_flush(void);
int channel_close(void);

#endif
