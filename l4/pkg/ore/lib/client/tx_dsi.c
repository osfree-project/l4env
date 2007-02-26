#include "local.h"
#include <stdlib.h>
#include <l4/env/errno.h>

void *l4ore_get_send_area(int handle)
{
    return descriptor_table[handle].send_addr;
}

// We assume that the data pointer points to a valid area inside
// the send dataspace. DSI will check this.
int ore_send_dsi(l4ore_handle_t channel, int handle,
                 char *data, unsigned int size)
{
    dsi_packet_t *packet;
    dsi_socket_t *socket = descriptor_table[handle].local_send_socket;
    int ret;
    
    ret = dsi_packet_get(socket, &packet);
    if (ret)
    {
        LOG_Error("error on packet_get: %d", ret);
        LOG("%s", l4env_strerror(-ret));
        return -1;
    }
    
    ret = dsi_packet_add_data(socket, packet, data, size, 0);
    if (ret)
    {
        LOG_Error("error on add_data: %d", ret);
        LOG("%s", l4env_strerror(-ret));
        return -1;
    }
    
    ret = dsi_packet_commit(socket, packet);
    if (ret)
    {
        LOG_Error("error on packet_commit: %d", ret);
        LOG("%s", l4env_strerror(-ret));
        return -1;
    }
    
    return 0;
}
