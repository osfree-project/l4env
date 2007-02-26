/*!
 * \file	log/server/src/flusher.h
 * \brief	Log-Server, functions dealing with flushing the buffer
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_SERVER_SRC_FLUSHER_H_
#define __LOG_SERVER_SRC_FLUSHER_H_

extern int flusher_prio;
extern l4_threadid_t main_thread, flusher_thread;

extern int flusher_init(void);
extern int flush_buffer(void);
extern int do_flush_buffer(void);

#endif
