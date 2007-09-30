/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-amd64/L4API-l4v2/ipc.h
 * \brief   L4 IPC System Calls
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_IPC_H__
#define __L4_IPC_H__

#include <l4/sys/types.h>

#define L4_IPC_IOMAPMSG_BASE 0xfffffffff0000000
#include <l4/sys/consts_ipc.h>

/*****************************************************************************
 *** IPC parameters
 *****************************************************************************/

/**
 * Structure used to describe destination and true source if a chief
 * wants to deceit
 * \ingroup api_types_id
 */
typedef struct {
  l4_threadid_t dest;         ///< IPC destination thread id
  l4_threadid_t true_src;     ///< IPC source thread id
} l4_ipc_deceit_ids_t;


/*****************************************************************************
 *** IPC calls
 *****************************************************************************/

/**
 * IPC Call, usual blocking RPC
 * \ingroup api_calls_ipc
 *
 * \param   dest         Thread id of the call destination
 * \param   snd_msg      Pointer to the send message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_NIL_DESCRIPTOR the IPC does not include a
 *                         send operation.
 *                       - #L4_IPC_SHORT_MSG the IPC includes sending a message
 *                         to the destination specified by \a dest. The message
 *                         consists solely of the two 32-bit words \a snd_dword0
 *                         and \a snd_dword1.
 *                       - \c \<mem\> the IPC includes sending a message to the
 *                         destination specified by \a dest. \a snd_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). The first two 32-bit
 *                         words of the message have to be given separately in
 *                         \a snd_dword0 and \a snd_dword1.
 * \param   snd_dword0   The first dword to be transmitted.
 * \param   snd_dword1   The second dword to be transmitted.
 * \param   rcv_msg      Pointer to the receive message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_NIL_DESCRIPTOR the IPC does not include a
 *                         receive operation.
 *                       - #L4_IPC_SHORT_MSG Only messages up to two 32-bit
 *                         words and are accepted. The received message is
 *                         returned in \a rcv_dword0 and \a rcv_dword1.
 *                       - \c \<mem\> Receive message. \a rcv_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). Note that the first
 *                         two 32-bit words of the received message are returned
 *                         in \a rcv_dword0 and \a rcv_dword1.
 *                       - #L4_IPC_MAPMSG(address,size) Only a flexpage or up to
 *                         two 32-bit words (in \a rcv_dword0 and \a rcv_dword1)
 *                         are accepted.
 *                       - #L4_IPC_IOMAPMSG(port, iosize) Only an I/O flexpage
 *                         or up to two 32-bit words (in \a rcv_dword0 and
 *                         \a rcv_dword1) are accepted.
 * \retval  rcv_dword0   The first dword of the received message,
 *                       undefined if no message was received.
 * \retval  rcv_dword1   The second dword of the received message,
 *                       undefined if no message was received.
 * \param   timeout      IPC timeout (see #l4_ipc_timeout).
 * \retval  result       Result message dope
 *
 * \return  0 if no error occurred. The send operation (if specified) was
 *          successful, and if a receive operation was also specified, a
 *          message was also received correctly. != 0 if an error occurred:
 *          - #L4_IPC_ENOT_EXISTENT Non-existing destination or source.
 *          - #L4_IPC_RETIMEOUT Timeout during receive operation.
 *          - #L4_IPC_SETIMEOUT Timeout during send operation.
 *          - #L4_IPC_RECANCELED Receive operation canceled by another thread.
 *          - #L4_IPC_SECANCELED Send operation canceled by another thread.
 *          - #L4_IPC_REMAPFAILED Map failed due to a shortage of page
 *            tables during receive operation.
 *          - #L4_IPC_SEMAPFAILED Map failed due to a shortage of page
 *            tables during send operation.
 *          - #L4_IPC_RESNDPFTO Send pagefault timeout.
 *          - #L4_IPC_SERCVPFTO Receive pagefault timeout.
 *          - #L4_IPC_REABORTED Receive operation aborted by another thread.
 *          - #L4_IPC_SEABORTED Send operation aborted by another thread.
 *          - #L4_IPC_REMSGCUT Received message cut. Potential reasons are:
 *            -# The recipient's mword buffer is too small.
 *            -# The recipient does not accept enough strings.
 *            -# At least one of the recipient's string buffers is too small.
 *
 * \a snd_msg is sent to thread with id \a dest and the invoker waits for a
 * reply from this thread. Messages from other sources are not accepted.
 * Note that since the send/receive transition needs no time, the destination
 * can reply with send timeout 0.
 *
 * This operation can also be used for a server with one dedicated client. It
 * sends the reply to the client and waits for the client's next order.
 */
L4_INLINE int
l4_ipc_call(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

L4_INLINE int
l4_ipc_call_tag(l4_threadid_t dest,
                const void *snd_msg,
                l4_umword_t snd_w0,
                l4_umword_t snd_w1,
                l4_msgtag_t tag,
                void *rcv_msg,
                l4_umword_t *rcv_w0,
                l4_umword_t *rcv_w1,
                l4_timeout_t timeout,
                l4_msgdope_t *result,
                l4_msgtag_t *rtag);

/**
 * IPC reply and wait, send a reply to a client and wait for next message
 * \ingroup api_calls_ipc
 *
 * \param   dest         Thread id of the send destination
 * \param   snd_msg      Pointer to the send message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_NIL_DESCRIPTOR the IPC does not include a
 *                         send operation.
 *                       - #L4_IPC_SHORT_MSG the IPC includes sending a message
 *                         to the destination specified by \a dest. The message
 *                         consists solely of the two 32-bit words \a snd_dword0
 *                         and \a snd_dword1.
 *                       - \c \<mem\> the IPC includes sending a message to the
 *                         destination specified by \a dest. \a snd_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). The first two 32-bit
 *                         words of the message have to be given separately in
 *                         \a snd_dword0 and \a snd_dword1.
 * \param   snd_dword0   The first dword to be transmitted.
 * \param   snd_dword1   The second dword to be transmitted.
 * \retval  src          Source thread id of the received message
 * \param   rcv_msg      Pointer to the receive message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_NIL_DESCRIPTOR the IPC does not include a
 *                         receive operation.
 *                       - #L4_IPC_SHORT_MSG Only messages up to two 32-bit
 *                         words and are accepted. The received message is
 *                         returned in \a rcv_dword0 and \a rcv_dword1.
 *                       - \c \<mem\> Receive message. \a rcv_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). Note that the first
 *                         two 32-bit words of the received message are returned
 *                         in \a rcv_dword0 and \a rcv_dword1.
 *                       - #L4_IPC_MAPMSG(address,size) Only a flexpage or up to
 *                         two 32-bit words (in \a rcv_dword0 and \a rcv_dword1)
 *                         are accepted.
 *                       - #L4_IPC_IOMAPMSG(port, iosize) Only an I/O flexpage
 *                         or up to two 32-bit words (in \a rcv_dword0 and
 *                         \a rcv_dword1) are accepted.
 * \retval  rcv_dword0   The first dword of the received message,
 *                       undefined if no message was received.
 * \retval  rcv_dword1   The second dword of the received message,
 *                       undefined if no message was received.
 * \param   timeout      IPC timeout (see #l4_ipc_timeout).
 * \retval  result       Result message dope
 *
 * \return  0 if no error occurred. The send operation (if specified) was
 *          successful, and if a receive operation was also specified, a
 *          message was also received correctly. != 0 if an error occurred:
 *          - #L4_IPC_ENOT_EXISTENT Non-existing destination or source.
 *          - #L4_IPC_RETIMEOUT Timeout during receive operation.
 *          - #L4_IPC_SETIMEOUT Timeout during send operation.
 *          - #L4_IPC_RECANCELED Receive operation canceled by another thread.
 *          - #L4_IPC_SECANCELED Send operation canceled by another thread.
 *          - #L4_IPC_REMAPFAILED Map failed due to a shortage of page
 *            tables during receive operation.
 *          - #L4_IPC_SEMAPFAILED Map failed due to a shortage of page
 *            tables during send operation.
 *          - #L4_IPC_RESNDPFTO Send pagefault timeout.
 *          - #L4_IPC_SERCVPFTO Receive pagefault timeout.
 *          - #L4_IPC_REABORTED Receive operation aborted by another thread.
 *          - #L4_IPC_SEABORTED Send operation aborted by another thread.
 *          - #L4_IPC_REMSGCUT Received message cut. Potential reasons are:
 *            -# The recipient's mword buffer is too small.
 *            -# The recipient does not accept enough strings.
 *            -# At least one of the recipient's string buffers is too small.
 *
 * \a snd_msg is sent to thread with id \a dest and the invoker waits for a
 * reply from any source. This is the standard server operation: it sends
 * a reply to the actual client and waits for the next order which may
 * come from a different client.
 */
L4_INLINE int
l4_ipc_reply_and_wait(l4_threadid_t dest,
                      const void *snd_msg,
                      l4_umword_t snd_dword0,
                      l4_umword_t snd_dword1,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_dword0,
                      l4_umword_t *rcv_dword1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result);


L4_INLINE int
l4_ipc_reply_and_wait_tag(l4_threadid_t dest,
                          const void *snd_msg,
                          l4_umword_t snd_dword0,
                          l4_umword_t snd_dword1,
                          l4_msgtag_t tag,
                          l4_threadid_t *src,
                          void *rcv_msg,
                          l4_umword_t *rcv_dword0,
                          l4_umword_t *rcv_dword1,
                          l4_timeout_t timeout,
                          l4_msgdope_t *result,
                          l4_msgtag_t *rtag);


L4_INLINE int
l4_ipc_wait_next_period(l4_threadid_t *src,
			void *rcv_msg,
			l4_umword_t *rcv_dword0,
			l4_umword_t *rcv_dword1,
			l4_timeout_t timeout,
			l4_msgdope_t *result);


/**
 * IPC send, send a message to a thread
 * \ingroup api_calls_ipc
 *
 * \param   dest         Thread id of the send destination
 * \param   snd_msg      Pointer to the send message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_SHORT_MSG the IPC includes sending a message
 *                         to the destination specified by \a dest. The message
 *                         consists solely of the two 32-bit words \a snd_dword0
 *                         and \a snd_dword1.
 *                       - \c \<mem\> the IPC includes sending a message to the
 *                         destination specified by \a dest. \a snd_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). The first two 32-bit
 *                         words of the message have to be given separately in
 *                         \a snd_dword0 and \a snd_dword1.
 * \param   snd_dword0   The first dword to be transmitted.
 * \param   snd_dword1   The second dword to be transmitted.
 * \param   timeout      IPC timeout (see #l4_ipc_timeout).
 * \retval  result       Result message dope
 *
 * \return  0 if no error occurred. The send operation (if specified) was
 *          successful, and if a receive operation was also specified, a
 *          message was also received correctly. != 0 if an error occurred:
 *          - #L4_IPC_ENOT_EXISTENT Non-existing destination or source.
 *          - #L4_IPC_RETIMEOUT Timeout during receive operation.
 *          - #L4_IPC_SETIMEOUT Timeout during send operation.
 *          - #L4_IPC_RECANCELED Receive operation canceled by another thread.
 *          - #L4_IPC_SECANCELED Send operation canceled by another thread.
 *          - #L4_IPC_REMAPFAILED Map failed due to a shortage of page
 *            tables during receive operation.
 *          - #L4_IPC_SEMAPFAILED Map failed due to a shortage of page
 *            tables during send operation.
 *          - #L4_IPC_RESNDPFTO Send pagefault timeout.
 *          - #L4_IPC_SERCVPFTO Receive pagefault timeout.
 *          - #L4_IPC_REABORTED Receive operation aborted by another thread.
 *          - #L4_IPC_SEABORTED Send operation aborted by another thread.
 *          - #L4_IPC_REMSGCUT Received message cut. Potential reasons are:
 *            -# The recipient's mword buffer is too small.
 *            -# The recipient does not accept enough strings.
 *            -# At least one of the recipient's string buffers is too small.
 *
 * \a snd_msg is sent to the thread with id \a dest. There is no
 * receive phase included. The invoker continues working after sending
 * the message.
 */
L4_INLINE int
l4_ipc_send(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

L4_INLINE int
l4_ipc_send_tag(l4_threadid_t dest,
                const void *snd_msg,
                l4_umword_t w0,
                l4_umword_t w1,
                l4_msgtag_t tag,
                l4_timeout_t timeout,
                l4_msgdope_t *result);

/**
 * IPC wait, wait for message from any source
 * \ingroup api_calls_ipc
 *
 * \retval  src          Source thread id of the received message
 * \param   rcv_msg      Pointer to the receive message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_SHORT_MSG Only messages up to two 32-bit
 *                         words and are accepted. The received message is
 *                         returned in \a rcv_dword0 and \a rcv_dword1.
 *                       - \c \<mem\> Receive message. \a rcv_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). Note that the first
 *                         two 32-bit words of the received message are returned
 *                         in \a rcv_dword0 and \a rcv_dword1.
 *                       - #L4_IPC_MAPMSG(address,size) Only a flexpage or up to
 *                         two 32-bit words (in \a rcv_dword0 and \a rcv_dword1)
 *                         are accepted.
 *                       - #L4_IPC_IOMAPMSG(port, iosize) Only an I/O flexpage
 *                         or up to two 32-bit words (in \a rcv_dword0 and
 *                         \a rcv_dword1) are accepted.
 * \retval  rcv_dword0   The first dword of the received message.
 * \retval  rcv_dword1   The second dword of the received message.
 * \param   timeout      IPC timeout (see #l4_ipc_timeout).
 * \retval  result       Result message dope
 *
 * \return  0 if no error occurred. The send operation (if specified) was
 *          successful, and if a receive operation was also specified, a
 *          message was also received correctly. != 0 if an error occurred:
 *          - #L4_IPC_ENOT_EXISTENT Non-existing destination or source.
 *          - #L4_IPC_RETIMEOUT Timeout during receive operation.
 *          - #L4_IPC_SETIMEOUT Timeout during send operation.
 *          - #L4_IPC_RECANCELED Receive operation canceled by another thread.
 *          - #L4_IPC_SECANCELED Send operation canceled by another thread.
 *          - #L4_IPC_REMAPFAILED Map failed due to a shortage of page
 *            tables during receive operation.
 *          - #L4_IPC_SEMAPFAILED Map failed due to a shortage of page
 *            tables during send operation.
 *          - #L4_IPC_RESNDPFTO Send pagefault timeout.
 *          - #L4_IPC_SERCVPFTO Receive pagefault timeout.
 *          - #L4_IPC_REABORTED Receive operation aborted by another thread.
 *          - #L4_IPC_SEABORTED Send operation aborted by another thread.
 *          - #L4_IPC_REMSGCUT Received message cut. Potential reasons are:
 *            -# The recipient's mword buffer is too small.
 *            -# The recipient does not accept enough strings.
 *            -# At least one of the recipient's string buffers is too small.
 *
 * This operation includes no send phase. The invoker waits for a message
 * from any source (including a hardware interrupt).
 */
L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

L4_INLINE int
l4_ipc_wait_tag(l4_threadid_t *src,
                void *rcv_msg,
                l4_umword_t *rcv_dword0,
                l4_umword_t *rcv_dword1,
                l4_timeout_t timeout,
                l4_msgdope_t *result,
	        l4_msgtag_t *tag);

/**
 * IPC receive, wait for a message from a specified thread
 * \ingroup api_calls_ipc
 *
 * \param   src          Thread to receive message from
 * \param   rcv_msg      Pointer to the receive message descriptor. It can
 *                       contain the following values:
 *                       - #L4_IPC_SHORT_MSG Only messages up to two 32-bit
 *                         words and are accepted. The received message is
 *                         returned in \a rcv_dword0 and \a rcv_dword1.
 *                       - \c \<mem\> Receive message. \a rcv_msg must
 *                         point to a valid message buffer (see
 *                         \ref long_ipc "Long IPC"). Note that the first
 *                         two 32-bit words of the received message are returned
 *                         in \a rcv_dword0 and \a rcv_dword1.
 *                       - #L4_IPC_MAPMSG(address,size) Only a flexpage or up to
 *                         two 32-bit words (in \a rcv_dword0 and \a rcv_dword1)
 *                         are accepted.
 *                       - #L4_IPC_IOMAPMSG(port, iosize) Only an I/O flexpage
 *                         or up to two 32-bit words (in \a rcv_dword0 and
 *                         \a rcv_dword1) are accepted.
 * \retval  rcv_dword0   The first dword of the received message.
 * \retval  rcv_dword1   The second dword of the received message.
 * \param   timeout      IPC timeout (see #l4_ipc_timeout).
 * \retval  result       Result message dope
 *
 * \return  0 if no error occurred. The send operation (if specified) was
 *          successful, and if a receive operation was also specified, a
 *          message was also received correctly. != 0 if an error occurred:
 *          - #L4_IPC_ENOT_EXISTENT Non-existing destination or source.
 *          - #L4_IPC_RETIMEOUT Timeout during receive operation.
 *          - #L4_IPC_SETIMEOUT Timeout during send operation.
 *          - #L4_IPC_RECANCELED Receive operation canceled by another thread.
 *          - #L4_IPC_SECANCELED Send operation canceled by another thread.
 *          - #L4_IPC_REMAPFAILED Map failed due to a shortage of page
 *            tables during receive operation.
 *          - #L4_IPC_SEMAPFAILED Map failed due to a shortage of page
 *            tables during send operation.
 *          - #L4_IPC_RESNDPFTO Send pagefault timeout.
 *          - #L4_IPC_SERCVPFTO Receive pagefault timeout.
 *          - #L4_IPC_REABORTED Receive operation aborted by another thread.
 *          - #L4_IPC_SEABORTED Send operation aborted by another thread.
 *          - #L4_IPC_REMSGCUT Received message cut. Potential reasons are:
 *            -# The recipient's mword buffer is too small.
 *            -# The recipient does not accept enough strings.
 *            -# At least one of the recipient's string buffers is too small.
 *
 * This operation includes no send phase. The invoker waits for a message
 * from \a src. Messages from other sources are not accepted. Note
 * that also a hardware interrupt might be specified as source.
 */
L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1,
               l4_timeout_t timeout,
               l4_msgdope_t *result);

L4_INLINE int
l4_ipc_receive_tag(l4_threadid_t src,
                   void *rcv_msg,
                   l4_umword_t *rcv_w0,
                   l4_umword_t *rcv_w1,
                   l4_timeout_t timeout,
                   l4_msgdope_t *result,
                   l4_msgtag_t *tag);

/**
 * Sleep for an amount of time.
 * \ingroup api_calls_ipc
 *
 * \param   timeout      IPC timeout (see #l4_ipc_timeout).
 *
 * \return  error code:
 *          - #L4_IPC_RETIMEOUT Timeout during receive operation (expected!)
 *          - #L4_IPC_RECANCELED Receive operation canceled by another thread.
 *          - #L4_IPC_REABORTED Receive operation aborted by another thread.
 *
 * This operation includes no send phase. The invoker waits until the timeout
 * is expired or the IPC was aborted by another thread.
 */
L4_INLINE int
l4_ipc_sleep(l4_timeout_t timeout);

/**
 * Check if received message contains flexpage
 * \ingroup api_calls_ipc
 *
 * \param   msgdope      IPC result message dope
 * \return  != 0 if flexpage received, 0 if not
 */
L4_INLINE int
l4_ipc_fpage_received(l4_msgdope_t msgdope);

/**
 * Check if flexpage was granted
 * \ingroup api_calls_ipc
 *
 * \param   fp           Flexpage descriptor
 * \return  != 0 if flexpage was granted, 0 if not
 */
L4_INLINE int
l4_ipc_is_fpage_granted(l4_fpage_t fp);

/**
 * Check if flexpage is writable
 * \ingroup api_calls_ipc
 *
 * \param   fp           Flexpage descriptor
 * \return  != 0 if flexpage is writable, 0 if not
 */
L4_INLINE int
l4_ipc_is_fpage_writable(l4_fpage_t fp);


/*
 * Internal defines used to build IPC parameters for the L4 kernel
 */

#define L4_IPC_DECEIT           1
#define L4_IPC_OPEN_IPC         1

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

#include <l4/sys/ipc-invoke.h>

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef PROFILE
#  include "ipc-l42-profile.h"
#else
#  if GCC_VERSION < 303
#    error gcc >= 3.3 required
#  else
#    include "ipc-l42-gcc3.h"
#  endif
#endif

L4_INLINE int
l4_ipc_call(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_msgtag_t dummytag;
  return l4_ipc_call_tag(dest, snd_msg, snd_dword0, snd_dword1,
                         l4_msgtag(0,0,0,0), rcv_msg,
                         rcv_dword0, rcv_dword1, timeout, result, &dummytag);
}

L4_INLINE int
l4_ipc_reply_and_wait(l4_threadid_t dest,
                      const void *snd_msg,
                      l4_umword_t snd_dword0,
                      l4_umword_t snd_dword1,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_dword0,
                      l4_umword_t *rcv_dword1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result)
{
  l4_msgtag_t dummytag;
  return l4_ipc_reply_and_wait_tag(dest, snd_msg, snd_dword0, snd_dword1,
                                   l4_msgtag(0,0,0,0), src,
                                   rcv_msg, rcv_dword0, rcv_dword1,
                                   timeout, result, &dummytag);
}

L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_msgtag_t dummytag;
  return l4_ipc_wait_tag(src, rcv_msg, rcv_dword0, rcv_dword1, timeout, result,
                         &dummytag);
}

L4_INLINE int
l4_ipc_send(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  return l4_ipc_send_tag(dest, snd_msg, snd_dword0, snd_dword1,
                         l4_msgtag(0,0,0,0), timeout, result);
}

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  l4_msgtag_t dummytag;
  return l4_ipc_receive_tag(src, rcv_msg, rcv_dword0, rcv_dword1, timeout,
                            result, &dummytag);
}

L4_INLINE int
l4_ipc_sleep(l4_timeout_t timeout)
{
  l4_umword_t dummy;
  l4_msgdope_t result;
  return l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
                        timeout, &result);
}

L4_INLINE int
l4_ipc_fpage_received(l4_msgdope_t msgdope)
{
  return msgdope.md.fpage_received != 0;
}

L4_INLINE int
l4_ipc_is_fpage_granted(l4_fpage_t fp)
{
  return fp.fp.grant != 0;
}

L4_INLINE int
l4_ipc_is_fpage_writable(l4_fpage_t fp)
{
  return fp.fp.write != 0;
}


#endif /* !__L4_IPC_H__ */
