/*!
 * \file   log/server/src/muxed.h
 * \brief  Functions and wire protocol definitions for muxed channels
 *
 * \date   10/05/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_SERVER_SRC_MUXED_H_
#define __LOG_SERVER_SRC_MUXED_H_

/* define protocol version tag */
#define LOG_PROTOCOL_VERSION  0x01

/* Type constants */
#define LOG_TYPE_UNKNOWN  1
#define LOG_TYPE_DATA     2
#define LOG_TYPE_LOG      3

/* Flags - none used at the moment */
#define LOG_FLAG_NONE     0

/* the header */
struct muxed_header {
	unsigned char  version;
	unsigned char  channel;
	unsigned char  type;
	unsigned char  flags;
	unsigned int   data_length;

};

extern l4_fpage_t channel_get_next_fpage(void);
extern int channel_open(l4_fpage_t page, int channel);
extern int channel_write(int id, unsigned int offset, unsigned int size);
extern int channel_flush(int id);
extern int channel_close(int id);

#endif /* __LOG_SERVER_SRC_MUXED_H */
