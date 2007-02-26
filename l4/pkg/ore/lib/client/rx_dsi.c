/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "local.h"
#include <stdlib.h>
#include <dice/dice.h>

void *l4ore_get_recv_area(int handle)
{
    return descriptor_table[handle].recv_addr;
}

/* Blocking receive(). The timeout parameter is currently ignored for
 * the DSI version, because we cannot abort an ongoing packet_get()
 * with DSI. :(
 */
int ore_recv_dsi_blocking(l4ore_handle_t channel, int handle,
                          char **data, l4_size_t *size, 
                          l4_timeout_t timeout)
{
    dsi_socket_t *sock  = NULL;
    dsi_packet_t *pack  = NULL;
    int ret;
    
    sock = descriptor_table[handle].local_recv_socket;
    
    // set socket to blocking mode
    dsi_socket_set_flags(sock, DSI_SOCKET_BLOCK);
    
    ret = dsi_packet_get(sock, &pack);
    if (ret)
    {
        LOG_Error("error on blocking receive: %d (%s)", ret,
            l4env_strerror(-ret));
        return ret;
    }
    
    ret = dsi_packet_get_data(sock, pack, (void **)data, size);
    if (ret)
    {
        LOG_Error("error on packet_get_data: %d (%s)", ret,
            l4env_strerror(-ret));
        return ret;
    }

    __l4ore_remember_packet(sock, pack, *data, *size);
    
    return 0;
}

int ore_recv_dsi_nonblocking(l4ore_handle_t channel, int handle,
                             char **data, l4_size_t *size)
{
    dsi_socket_t *sock  = NULL;
    dsi_packet_t *pack  = NULL;
    int ret;
    
    sock = descriptor_table[handle].local_recv_socket;
    
    // set socket to non-blocking mode
    dsi_socket_clear_flags(sock, DSI_SOCKET_BLOCK);
    
    ret = dsi_packet_get(sock, &pack);
    if (ret)
    {
        LOG_Error("error on non-blocking receive: %d (%s)", ret,
            l4env_strerror(-ret));
        return ret;
    }
    
    ret = dsi_packet_get_data(sock, pack, (void **)data, size);
    if (ret)
    {
        LOG_Error("error on packet_get_data: %d (%s)", ret,
            l4env_strerror(-ret));
        return ret;
    }

    __l4ore_remember_packet(sock, pack, *data, *size);
    
    return 0;
}
