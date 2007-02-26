/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef __ORE_H
#define __ORE_H

#include <l4/ore/ore-types.h>
#include <l4/dm_generic/types.h>

/* distinguish blocking and non-blocking calls */
#define ORE_BLOCKING_CALL           1
#define ORE_NONBLOCKING_CALL        0

/*!\brief Open a connection to ORe.
 * \ingroup management
 *
 * \param device    name of the device to open (e.g. "eth0")
 * \return mac      Space for MAC address provided by server
 * \param conf      initial configuration for this connection
 *
 * \return          connection handle
 *                  <0 for failure
 */
int l4ore_open(char *device, unsigned char mac[6], l4ore_config *conf);

/*!\brief Send packet through the connection specified.
 * \ingroup send_receive
 *
 * \param handle    connection handle
 * \param buf       packet buffer
 * \param size      packet size
 *
 * \return          0 succes
 * \return          <0 error
 */
int l4ore_send(int handle, char *buf, l4_size_t size);

/*!\brief Receive packet. Block until a packet is available.
 * \ingroup send_receive
 *
 * \param handle    connection handle
 * \param buf       receive buffer
 * \param size      receive buffer size
 * \param timeout   IPC timeout
 *
 * \return          0 success
 * \return          <0 error
 * \return          >0 if buffer is too small, the return value specifies
 *                  the buffer size needed.
 */
int l4ore_recv_blocking(int handle, char **buf, l4_size_t *size, l4_timeout_t timeout);

/*!\brief Receive packet. Return immediately when no data available.
 * \ingroup send_receive
 *
 * \param handle    connection handle
 * \param buf       receive buffer
 * \param size      receive buffer size
 *
 * \return          0 success
 * \return          <0 error
 * \return          >0 if buffer is too small, the return value specifies
 *                  the buffer size needed.
 */
int l4ore_recv_nonblocking(int handle, char **buf, unsigned int *size);

/*!\brief Close connection.
 * \ingroup management
 *
 * \param handle    Connection handle.
 */
void l4ore_close(int handle);

/*!\brief Set configuration for connection. Note that ro values in the
 *        connection descriptor will be ignored.
 * \ingroup management
 *
 * \param handle    connection handle
 * \param new_desc  new connection configuration
 */
void l4ore_set_config(int handle, l4ore_config *conf);

/*!\brief Get configuration for connection.
 * \ingroup management
 *
 * \param handle    connection handle
 *
 * \return          The current configuration info for this channel.
 */
l4ore_config l4ore_get_config(int handle);

/*!\brief Set/unset debugging.
 * \ingroup debug
 *
 * \param conf      config data
 * \param flag      Debugging flag.
 */
void l4ore_debug(l4ore_config *conf, int flag);

/*!\brief Get address of the send dataspace.
 * \ingroup sharedmem
 * 
 * \param handle    connection handle
 */
void *l4ore_get_send_area(int handle);

/*!\brief Get address of the receive dataspace.
 * \ingroup sharedmem
 *
 * \param handle    connection handle
 */
void *l4ore_get_recv_area(int handle);

/*!\brief Return packet at address addr to the DSI pool.
 * \ingroup sharedmem
 * 
 * \param addr      packet address
 */
void l4ore_packet_to_pool(void *addr);
#endif
