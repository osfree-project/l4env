#include "ore-local.h"

/* netif_rx() function for the case where the client wants to receive
 * data through a dataspace managed by the DSI.
 */
int netif_rx_dsi(int h, struct sk_buff *skb)
{
    dsi_packet_t *pack;
    dsi_socket_t *sock = ore_connection_table[h].rx_socket;
    int ret;
    
    // get packet
    ret = dsi_packet_get(sock, &pack);
    if (ret == -DSI_ENOPACKET)
    {
        LOGd(ORE_DEBUG_DSI, "no more packets available for client %d", h);
        LOGd(ORE_DEBUG_DSI, "dropping packet.");
        return NET_RX_SUCCESS;
    }
    else if (ret)
    {
        LOG_Error("error on dsi_packet_get : %d", ret);
        return NET_RX_SUCCESS;
    }
        
    // copy data from skb to data area
    memcpy(ore_connection_table[h].rx_addr, skb->data, skb->len);
    
    // packet_add_data
    ret = dsi_packet_add_data(sock, pack, ore_connection_table[h].rx_addr,
        skb->len, 0);
    if (ret)
    {
        LOG_Error("error on packet_add_data: %d (%s)", ret,
            l4env_strerror(-ret));
        return NET_RX_SUCCESS;
    }
    
    // packet_commit
    ret = dsi_packet_commit(sock, pack);
    if (ret)
    {
        LOG_Error("error on dsi_commit: %d (%s)", ret,
            l4env_strerror(-ret));
    }
    
    // packets are at most as big as the device's mtu -> add this to address
    // to be safe
    ore_connection_table[h].rx_addr += ore_connection_table[h].dev->mtu;
    
    // wrap around if necessary - this should not overwrite data as long as
    // the rx dataspace is large enough
    // TODO: check DS size at open() !
    void *max_addr = ore_connection_table[h].rx_start 
                     + ore_connection_table[h].rx_size
                     - ore_connection_table[h].dev->mtu;
    if (ore_connection_table[h].rx_addr >= max_addr)
        ore_connection_table[h].rx_addr = ore_connection_table[h].rx_start;
    
    return NET_RX_SUCCESS;
}
