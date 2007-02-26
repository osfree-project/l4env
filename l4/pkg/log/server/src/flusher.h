/*!
 * \file	log/server/src/flusher.h
 * \brief	Log-Server, functions dealing with flushing the buffer
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#ifndef __LOG_SERVER_SRC_FLUSHER_H_
#define __LOG_SERVER_SRC_FLUSHER_H_

extern l4_threadid_t main_thread, flusher_thread;

extern int flusher_init(int prio);
extern int flush_buffer(void);
extern int do_flush_buffer(void);

#endif
