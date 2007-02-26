/*!
 * \file   log/include/ARCH-x86/server.h
 * \brief  Logging facility - functions for server communication
 *
 * \date   03/14/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 */

#ifndef __LOG_INCLUDE_ARCH_X86_SERVER_H_
#define __LOG_INCLUDE_ARCH_X86_SERVER_H_

#if ! ( defined L4API_l4v2 || defined L4API_l4x0 )
#error This file must not be directly included
#endif

#include <l4/sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!\brief variable containing the ID of the logserver.
 *
 * If you use the logserver, but you do not want the lib to call
 * the nameserver for resolving names, you can set the id directly.
 */
extern void LOG_server_setid(l4_threadid_t id);

/*!\brief Open a binary output channel
 *
 * \ingroup muxed_api
 *
 * This function opens a binary output channel at the logserver. Multiple
 * clients/programs can send data to the same output channel at the logserver,
 * the output is merged then. To do so, the clients must specify the same
 * channel nr when opening. A reference counter holds the number of
 * clients that opened that channel. Channel 1 is used for sending the
 * standard logging text on, using it for sending binary data is not
 * recommended.
 *
 * The caller of this function must specify a flexpage which is mapped to the
 * logserver. This flexpage is used for transfering data later when using
 * LOG_channel_write().
 *
 * \param  channel      channel nr of the channel to open
 * \param  fpage        a flexpage that will later be used for writing data.
 *			The flexpage must contain pinned pages prior to
 *			calling this function. The flexpage must not exceed
 *			2MB in its size.
 *
 * \retval >=0		the id of the connection to use later
 * \retval -L4_ENOMEM	the specified fpage exceeded the maximum size
 * \retval -L4_ENOMAP	if the server did not have a free area to receive the
 *			fpage or is otherwise short on resources
 * \retval -L4_EIPC	some problem with server communication occured
 * \retval -L4_EBUSY	the server is configured using normal mode,
 *                      not multiplexed mode for TCP-output. Sending binary
 *                      data to the client requires multiplexed output.  See
 *                      \ref p_server on how to configure multiplexed mode.
 *
 * \see  LOG_channel_write(), LOG_channel_close().
 * \note This function is only available when using the logserver!
 */
extern int LOG_channel_open(int channel, l4_fpage_t fpage);

/*!\brief Send data to a binary output channel
 *
 * \ingroup muxed_api
 *
 * This function sends data to an open binary output channel at the
 * logserver. This function does not wait until the data is actually
 * sent. Therefore, you should not overwrite the data beeing sent,
 * until a subsequently call to LOG_channel_flush() returns.
 *
 * Multiple clients/programs can send to the same output channel at
 * the logserver, the output is merged then. It is ensured, that data
 * of different calls to LOG_channel_write() does not intercept each
 * other. When multiple clients/programs use the same channel, it is
 * recommended to use an additional encapsulation to demultiplex the
 * data lateron.
 *
 * \param  id	 id of the connection as returned from LOG_channel_open()
 * \param  off   offset of the data in the flexpage provided on
 *		 LOG_channel_open()
 * \param  size  number of bytes to write
 *
 * \retval 0  no error
 * \retval <0 in the case of error
 *
 * \see  LOG_channel_open(), LOG_channel_flush(), LOG_channel_close().
 * \note This function is only available when using the logserver! */
extern int LOG_channel_write(int id, unsigned off, unsigned size);


/*!\brief Wait until data is sent by the logserver
 *
 * \ingroup muxed_api
 *
 * This function waits until the prior write-request is fulfilled and the
 * according memory in the mapped fpage can be reused.
 *
 * \param  id	 id of the connection as returned from LOG_channel_open()
 *
 * \retval 0  no error
 * \retval <0 in the case of error
 *
 * \see  LOG_channel_write.
 * \note This function is only available when using the logserver!
 */
extern int LOG_channel_flush(int id);


/*!\brief Close a binary output channel
 *
 * \ingroup muxed_api
 *
 * This function removes the mapping and decrements the reference counter to
 * an open binary output channel at the logserver. If the reference counter
 * becomes 0, the channel is actually closed, i.e. the resources are freed
 * in the logserver.
 *
 * \param  id	 id of the connection as returned from LOG_channel_open()
 *
 * \retval 0  no error
 * \retval <0 in the case of error
 *
 * \see  LOG_channel_open(), LOG_channel_write(), LOG_channel_flush().
 * \note This function is only available when using the logserver!
 */
extern int LOG_channel_close(int id);

#ifdef __cplusplus
}
#endif
#endif
