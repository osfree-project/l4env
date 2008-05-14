/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

/* Header file for the uIP-ORe library. This lib enables clients to communicate
 * through IP/UDP using the uIP stack and ORe. Originally uIP is not built as a
 * library, but needs to be linked statically against the application. 
 * This lib (libuip-ore.a) intents to solve this problem.
 */

#ifndef __UIP_ORE_H
#define __UIP_ORE_H

#include <l4/sys/linkage.h>
#include <arpa/inet.h>

//!\brief Configuration for the uIP library.
typedef struct uip_ore_config
{
    char ip[16];    //!< IP address
    unsigned short port_nr; //!< port number to use

    /* Callback functions for uIP events 
     * =================================
     */
    /*!\brief Received a packet. 
     * \param buf   the packet data
     * \param size  data size in bytes
     * \param port  port the data was received through
     *
     * NOTE: buf is invalid after returning from this function, so you
     *       need to make a copy if necessary.
     */
    L4_CV void (*recv_callback)(const void *buf, const unsigned size, unsigned port);    
    
    /*!\brief  Send acknowledgement received. 
     *
     * This is called so that the user is able to free dynamically allocated 
     * buffers after they have been successfully sent.
     * 
     * NOTE: When closing the connection, all pending data is dropped. For each
     *      dropped packet the ack_callback() is also called, because also these
     *      packets might be needed to be freed.
     * 
     * \param addr  address of the buffer that has been acknowledged/dropped.
     * \param port  port this packet was sent through
     */
    L4_CV void (*ack_callback)(void *addr, unsigned port);
    
    /*!\brief Retransmit of message necessary. 
     *
     * This could also be done by the library,
     * but the callback shall give you the opportunity to react upon such situations.
     *
     * \param addr  packet needing to be retransmitted. Just call uip_ore_send()
     *              again. 
     * \param size  size of packet
     * \param port  port to resend to
     */
    L4_CV void (*rexmit_callback)(void *addr, unsigned size, unsigned port);
    
    /*!\brief Connection established.
     *
     * \param ip    IP address remote host
     * \param port  remote port
     */
    L4_CV void (*connect_callback)(const struct in_addr ip, unsigned port);
    
    /*!\brief Connection has been aborted. 
     * \param port  remote port
     */
    L4_CV void (*abort_callback)(unsigned port);

    /*!\brief Connection timed out. 
     * \param port  remote port
     */
    L4_CV void (*timeout_callback)(unsigned port);

    /*!\brief Connection closed. 
     * \param port  remote port
     */
    L4_CV void (*close_callback)(unsigned port);

    /*!\brief Periodic poll callback. 
     *
     * uIP periodically polls the server callback function every 500 ms. If
     * you want to do something upon this period, this is the callback to
     * do so.
     */
    L4_CV void (*poll_callback)(void);
} uip_ore_config;

/*!\brief Config function for the uip_ore library.
 * \param conf  configuration data
 */
L4_CV void uip_ore_initialize(uip_ore_config *conf);

/*!\brief Thread function for the uip-ore library. The library needs to run in a 
 * new thread. 
 */
L4_CV void uip_ore_thread(void *arg);

/*!\brief Send a packet. 
 *
 * This function does however not actually send data, but
 * enqueues it into the uIP send list. The main thread will later on make
 * sure that this data is sent.
 *
 * NOTE: The packed will be truncated if it is too long for the underlying
 *       network layer.
 * 
 * NOTE: After the packet has been acknowledged, the ack_callback() function
 *       will be called with the address of the send buffer. If you have allocated
 *       the send buffer dynamically, this is the point where you should free
 *       the buffer.
 *
 * \param   buf     send buffer
 * \param   size    buffer size
 */
L4_CV void uip_ore_send(const char *buf, unsigned size, unsigned port);

/*!\brief Connect to a given IP and port number.
 *
 * \param ip    IP address of remote host
 * \param port  port at remote host
 *
 * \return  0   connecting
 * \return  !=0 error
 */
L4_CV int uip_ore_connect(struct in_addr ip, unsigned port);

/*!\brief Close connection on a given local port.
 *
 * \param port  local port number
 */
L4_CV void uip_ore_close(unsigned port);

#endif
