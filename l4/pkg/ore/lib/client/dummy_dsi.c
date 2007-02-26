/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "local.h"
#include "list.h"

void __l4ore_remember_packet(dsi_socket_t *sock, dsi_packet_t *pack, void *addr, l4_size_t size)
{
}

void l4ore_packet_to_pool(void *addr)
{
}

/* create DSI connection for sending data */
int __l4ore_init_send_socket(l4ore_handle_t server, l4ore_config *conf, dsi_socket_t **sock, void **addr)
{
    return -1;
}

int __l4ore_init_recv_socket(l4ore_handle_t server, l4ore_config *conf, dsi_socket_t **sock, void **addr)
{
    return -1;
}

void *l4ore_get_send_area(int h)
{
    return NULL;
}

void *l4ore_get_recv_area(int h)
{
    return NULL;
}

int ore_send_dsi(l4ore_handle_t channel, int handle, char *data, l4_size_t size)
{
    return -L4_ENOTSUPP;
}

int ore_recv_dsi_blocking(l4ore_handle_t channel, int handle, char **data, l4_size_t *size, l4_timeout_t t)
{
    return -L4_ENOTSUPP;
}

int ore_recv_dsi_nonblocking(l4ore_handle_t channel, int handle, char **data, l4_size_t *size)
{
    return -L4_ENOTSUPP;
}
