/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden				*
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "ore-local.h"

/* netif_rx() function for the case where the client wants to receive
 * data through a dataspace managed by the DSI.
 */
int netif_rx_dsi(int h, struct sk_buff *skb)
{
    return NET_RX_SUCCESS;
}

int __l4ore_in_dataspace(void *addr, dsi_packet_t **packet,
	dsi_socket_t **socket)
{
    return 0;
}

void __l4ore_do_packet_commit(dsi_packet_t *pack, dsi_socket_t *sock)
{
}


void __l4ore_remember_packet(dsi_packet_t *pack, dsi_socket_t *sock,
	void *addr, l4_size_t size)
{
}

int init_dsi_sendingclient(int channel, l4ore_config *conf)
{
    return -L4_ENOTSUPP;
}

int init_dsi_receivingclient(int channel, l4ore_config *conf)
{
    return -L4_ENOTSUPP;
}
