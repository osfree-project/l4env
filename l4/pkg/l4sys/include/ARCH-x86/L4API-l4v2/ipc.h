/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/ipc.h
 * \brief   L4 IPC System Calls
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_IPC_H__
#define __L4_IPC_H__

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

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

/**
 * Nil message descriptor
 * \ingroup api_types_msg
 * \hideinitializer
 */
#define L4_IPC_NIL_DESCRIPTOR   (-1)

/**
 * Short IPC (register-only) message descriptor
 * \ingroup api_types_msg
 * \hideinitializer
 */
#define L4_IPC_SHORT_MSG        0

/**
 * Short flexpage IPC (register-only) message descriptor, message words are
 * interpreted as send flexpage descriptor
 * \ingroup api_types_msg
 * \hideinitializer
 */
#define L4_IPC_SHORT_FPAGE      ((void *)2)

#define L4_IPC_STRING_SHIFT     8    /**< \ingroup api_types_msg */
#define L4_IPC_DWORD_SHIFT      13   /**< \ingroup api_types_msg */

/**
 * Build IPC message dope
 * \ingroup api_types_msg
 * \hideinitializer
 *
 * \param   dwords       Number of dwords in message
 * \param   strings      Number of indirect strings in message
 */
#define L4_IPC_DOPE(dwords, strings) \
( (l4_msgdope_t) {md: {0, 0, 0, 0, 0, 0, strings, dwords }})

/**
 * Build IPC timeout
 * \ingroup api_types_timeout
 * \hideinitializer
 *
 * \param   snd_man      Send timeout mantissa
 * \param   snd_exp      Send timeout exponent
 * \param   rcv_man      Receive timeout mantissa
 * \param   rcv_exp      Receive timeout exponent
 * \param   snd_pflt     Send pagefault timeout
 * \param   rcv_pflt     Receive pagefault timeout
 */
#define L4_IPC_TIMEOUT(snd_man, snd_exp, rcv_man, rcv_exp, snd_pflt, rcv_pflt)\
     ( (l4_timeout_t) \
       {to: { rcv_exp, snd_exp, rcv_pflt, snd_pflt, snd_man, rcv_man } } )

/**
 * IPC special timeouts
 * \ingroup api_types_timeout
 * \hideinitializer
 */
#define L4_IPC_NEVER			((l4_timeout_t) {timeout: 0})
#define L4_IPC_NEVER_INITIALIZER	{timeout: 0}
#define L4_IPC_RECV_TIMEOUT_0		L4_IPC_TIMEOUT(0,0,0,1,0,0)
#define L4_IPC_SEND_TIMEOUT_0		L4_IPC_TIMEOUT(0,1,0,0,0,0)
#define L4_IPC_BOTH_TIMEOUT_0		L4_IPC_TIMEOUT(0,1,0,1,0,0)

/**
 * Build short flexpage receive message descriptor.
 * \ingroup api_types_msg
 * \hideinitializer
 *
 * \param   address      Flexpage receive window address
 * \param   size         Receive window size (log2)
 */
#define L4_IPC_MAPMSG(address, size)  \
     ((void *)(l4_umword_t)( ((address) & L4_PAGEMASK) | ((size) << 2) \
                             | (unsigned long)L4_IPC_SHORT_FPAGE))

/**
 * Build short I/O flexpage receive message descriptor
 * \ingroup api_types_msg
 * \hideinitializer
 *
 * \param   port         I/O flexpage receive window base port
 * \param   iosize       Receive window size
 */
#define L4_IPC_IOMAPMSG(port, iosize)  \
     ((void *)(l4_umword_t)( 0xf0000000 | ((port) << 12) | ((iosize) << 2) \
                             | (unsigned long)L4_IPC_SHORT_FPAGE))



/*****************************************************************************
 *** IPC result checking
 *****************************************************************************/

#define L4_IPC_ERROR_MASK       0xF0    /**< \ingroup api_types_msg */
#define L4_IPC_DECEIT_MASK      0x01    /**< \ingroup api_types_msg */
#define L4_IPC_FPAGE_MASK       0x02    /**< \ingroup api_types_msg */
#define L4_IPC_REDIRECT_MASK    0x04    /**< \ingroup api_types_msg */
#define L4_IPC_SRC_MASK         0x08    /**< \ingroup api_types_msg */
#define L4_IPC_SND_ERR_MASK     0x10    /**< \ingroup api_types_msg */

/**
 * Test if IPC error occurred
 * \ingroup api_calls_ipc
 * \hideinitializer
 *
 * \param   x            IPC result message dope
 * \return  != 0 if error occurred, 0 if not
 */
#define L4_IPC_IS_ERROR(x)		(((x).msgdope) & L4_IPC_ERROR_MASK)

/**
 * Test if received message was deceited by a chief task
 * \ingroup api_calls_ipc
 * \hideinitializer
 *
 * \param   x            IPC result message dope
 * \return  != 0 if message was deceited, 0 if not
 */
#define L4_IPC_MSG_DECEITED(x) 		(((x).msgdope) & L4_IPC_DECEIT_MASK)

/**
 * Test if the message was redirected to a chief task
 * \ingroup api_calls_ipc
 * \hideinitializer
 *
 * \param   x            IPC result message dope
 * \return  != 0 if message was redirected, 0 if not
 */
#define L4_IPC_MSG_REDIRECTED(x)	(((x).msgdope) & L4_IPC_REDIRECT_MASK)

/**
 * Test if the message comes from outside or inside the clan
 * \ingroup api_calls_ipc
 * \hideinitializer
 *
 * \param   x            IPC result message dope
 * \return  != 0 if the message comes from an inner clan, 0 if it comes from
 *          outside the own clan.
 */
#define L4_IPC_SRC_INSIDE(x)		(((x).msgdope) & L4_IPC_SRC_MASK)

/**
 * Test if send operation failed
 * \ingroup api_calls_ipc
 * \hideinitializer
 *
 * \param   x            IPC result message dope
 * \return  != 0 if send operation failed, 0 if not
 */
#define L4_IPC_SND_ERROR(x)		(((x).msgdope) & L4_IPC_SND_ERR_MASK)

/*****************************************************************************
 *** IPC results
 *****************************************************************************/

/**
 * \brief Get IPC error from IPC result message dope
 * \ingroup api_calls_ipc
 * \hideinitializer
 *
 * \param   x            IPC result message dope
 */
#define L4_IPC_ERROR(x)       (((x).msgdope) & L4_IPC_ERROR_MASK)

#define L4_IPC_ENOT_EXISTENT  0x10   /**< Non-existing destination or source
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_RETIMEOUT      0x20   /**< Timeout during receive operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SETIMEOUT      0x30   /**< Timeout during send operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_RECANCELED     0x40   /**< Receive operation canceled
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SECANCELED     0x50   /**< Send operation canceled
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_REMAPFAILED    0x60   /**< Map flexpage failed in receive
                                      **  operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SEMAPFAILED    0x70   /**< Map flexpage failed in send operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_RESNDPFTO      0x80   /**< Send-pagefault timeout in receive
                                      **  operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SESNDPFTO      0x90   /**< Send-pagefault timeout in send
                                      **  operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_RERCVPFTO      0xA0   /**< Receive-pagefault timeout in receive
                                      **  operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SERCVPFTO      0xB0   /**< Receive-pagefault timeout in send
                                      **  operation
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_REABORTED      0xC0   /**< Receive operation aborted
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SEABORTED      0xD0   /**< Send operation aborted
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_REMSGCUT       0xE0   /**< Cut receive message (due to
                                      **  (a) message buffer is too small,
                                      **  (b) not enough strings are accepted,
                                      **  (c) at least one string buffer is too
                                      **      small)
                                      **  \ingroup api_calls_ipc
                                      **/
#define L4_IPC_SEMSGCUT       0xF0   /**< Cut send message (due to
                                      **  (a) message buffer is too small,
                                      **  (b) not enough strings are accepted,
                                      **  (c) at least one string buffer is too
                                      **      small)
                                      **  \ingroup api_calls_ipc
                                      **/
/**
 * Short IPC (register-only) message descriptor w/o donation
 * \ingroup api_types_msg
 * \hideinitializer
 */
#define L4_IPC_SHORT_MSG_NODONATE ((void*)((unsigned)L4_IPC_SHORT_MSG|\
					   L4_IPC_DECEIT_MASK))

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
 * \param   timeout      IPC timeout (see #L4_IPC_TIMEOUT).
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
 * \param   timeout      IPC timeout (see #L4_IPC_TIMEOUT).
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
 * \param   timeout      IPC timeout (see #L4_IPC_TIMEOUT).
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
 * \param   timeout      IPC timeout (see #L4_IPC_TIMEOUT).
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
 * \param   timeout      IPC timeout (see #L4_IPC_TIMEOUT).
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

#include <l4/sys/rt_sched-proto.h>
#include <l4/sys/ipc-invoke.h>

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef PROFILE
#  include "ipc-l42-profile.h"
#else
#  if GCC_VERSION < 295
#    error gcc >= 2.95 required
#  elif GCC_VERSION < 302
#    ifdef __PIC__
#      include "ipc-l42-gcc295-pic.h"
#    else
#      include "ipc-l42-gcc295-nopic.h"
#    endif
#  else
#    ifdef __PIC__
#      include "ipc-l42-gcc3-pic.h"
#    else
#      include "ipc-l42-gcc3-nopic.h"
#    endif
#  endif
#endif

#endif /* !__L4_IPC_H__ */
