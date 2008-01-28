/* $Id$ */
/**
 * \file	con/server/src/config.h
 * \brief	con configuration macros
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

/* malloc */
#define CONFIG_MALLOC_MAX_SIZE	(128*1024)

/* vc */
#define CONFIG_MAX_VC		20		/* number of virtual consoles */
#define CONFIG_MAX_SBUF_SIZE	(4*256*256)	/* max string buffer */

/* We assume that a client does l4_ipc_call for requests => snd to 0.
 * We want to leave the main loop from time to time      => rcv to 50ms */
#define REQUEST_TIMEOUT		l4_ipc_timeout(0,1,780,6)

/* We want to push an event to a client (send-only IPC)
 * The event handler may be busy handling the last event => snd to 100ms. */
#define EVENT_TIMEOUT		l4_ipc_timeout(781,7,0,0)

#define CONFIG_MAX_CLIENTS      4
