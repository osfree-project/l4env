#ifndef __ORE_H
#define __ORE_H

#include <l4/ore/ore-types.h>
#include <l4/dm_generic/types.h>

/* distinguish blocking and non-blocking calls */
#define ORE_BLOCKING_CALL           1
#define ORE_NONBLOCKING_CALL        0

/*!\brief Open a connection to ORe.
 *
 * \param device    name of the device to open (e.g. "eth0")
 * \retval mac      Space for MAC address provided by server
 * \param send_ds   dataspace for sending packets,
 *                  set to NULL or L4DM_INVALID_DATASPACE
 *                  if you do not wish to use a dataspace connection
 * \param recv_ds   dataspace for receiving packets,
 *                  set to NULL or L4DM_INVALID_DATASPACE
 *                  if you do not wish to use a dataspace connection
 * \param flags     flags for this connection:
 *                      ORE_FLAG_WANT_BROADCAST: we want to receive broadcast packets
 *
 * \return          connection handle
 *                  <0 for failure
 */
l4ore_handle_t l4ore_open(char *device, unsigned char mac[6],
                          l4dm_dataspace_t *send_ds, l4dm_dataspace_t *recv_ds,
                          l4ore_config *conf);

/*!\brief Send packet through the connection specified.
 *
 * \param handle    connection handle
 * \param buf       packet buffer
 * \param size      packet size
 *
 * \retval          0 succes
 * \retval          <0 error
 */
int l4ore_send(l4ore_handle_t handle, char *buf, int size);

/*!\brief Receive packet. Block until a packet is available.
 *
 * \param handle    connection handle
 * \param buf       receive buffer
 * \param size      receive buffer size
 *
 * \retval          0 success
 * \retval          <0 error
 * \retval          >0 if buffer is too small, the return value specifies
 *                  the buffer size needed.
 */
int l4ore_recv_blocking(l4ore_handle_t handle, char **buf, int *size);

/*!\brief Receive packet. Return immediately when no data available.
 *
 * \param handle    connection handle
 * \param buf       receive buffer
 * \param size      receive buffer size
 *
 * \retval          0 success
 * \retval          <0 error
 * \retval          >0 if buffer is too small, the return value specifies
 *                  the buffer size needed.
 */
int l4ore_recv_nonblocking(l4ore_handle_t handle, char **buf, int *size);

/*!\brief Close connection.
 *
 * \param handle    Connection handle.
 */
void l4ore_close(l4ore_handle_t handle);

/*!\brief Set configuration for connection. Note that ro values in the
 *        connection descriptor will be ignored.
 *
 * \param handle    connection handle
 * \param new_desc  new connection configuration
 */
void l4ore_set_config(l4ore_handle_t handle, l4ore_config *conf);

/*!\brief Get configuration for connection.
 *
 * \param handle    connection handle
 *
 * \retval          The current configuration info for this channel.
 */
l4ore_config l4ore_get_config(l4ore_handle_t handle);

/*!\brief Set/unset debugging.
 *
 * \param flag      Debugging flag.
 */
void l4ore_debug(int flag);

#endif
