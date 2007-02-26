/*!
 * \file	log/server/src/config.h
 * \brief       Log-Server compile-time configuration
 *
 * \date	03/05/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_SERVER_SRC_CONFIG_H_
#define __LOG_SERVER_SRC_CONFIG_H_

//! The port-number we are listening on
#define PORT_NR 23

//! The size of the internal buffer to hold the logged data
#define OUTPUT_BUFFER_SIZE (128*1024)

//! The max. number of bootp retries
#define CONFIG_BOOTP_RETRIES	5

//! The timeout for bootp requests in ms
#define CONFIG_BOOTP_TIMEOUT	5000


/* Logging configuration follows.
 *
 * \note The logging the log-server does, is NOT forwarded to the
 *       tcpip-connection, it goes directly to the kernel-debug
 *       interface.
 */

//! Set CONFIG_LOG_LOCK to 1 if you want log-output on locking activities.
#define	CONFIG_LOG_LOCK		0

//! Set CONFIG_LOG_REQUESTS to 1 if you want to see client-log requests.
#define CONFIG_LOG_REQUESTS	0

//! Set CONFIG_LOG_RINGBUFFER if you want to log activities on the ringbuffer.
#define CONFIG_LOG_RINGBUFFER	0

//! Set CONFIG_LOG_TCPIP if you want logging of tcpip-related actions.
#define CONFIG_LOG_TCPIP	0

//! Set CONFIG_LOG_IPC to log the IPCs between the logserver's threads.
#define CONFIG_LOG_IPC		0

//! Set CONFIG_LOG_NOTICE if you want state-information (recommended).
#define CONFIG_LOG_NOTICE	1

#endif
