/*!
 * \file   log/server/include/log_comm.h
 * \brief  internal include for logserver and -lib
 *
 * \date   02/11/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#ifndef __LOG_SERVER_INCLUDE_LOG_COMM_H_
#define __LOG_SERVER_INCLUDE_LOG_COMM_H_

#define LOG_NAMESERVER_NAME "stdlogV05"

// maximal size of message to log
#define LOG_BUFFERSIZE 81

// and its log2
#define LOG_LOG2_CHANNEL_BUFFER_SIZE (21)
// maximal size of communication fpage - 2MB
#define LOG_CHANNEL_BUFFER_SIZE (1<<LOG_LOG2_CHANNEL_BUFFER_SIZE)

#define LOG_COMMAND_LOG			0
#define LOG_COMMAND_CHANNEL_OPEN	1
#define LOG_COMMAND_CHANNEL_WRITE	2
#define LOG_COMMAND_CHANNEL_FLUSH	3
#define LOG_COMMAND_CHANNEL_CLOSE	4

#define LOG_COMMAND_MASK		0x0000ffff
#define LOG_COMMAND_FLAG_FLUSH		0x00010000

#endif
