/*****************************************************************************
 * Helper functions regarding the use of DSI for client-ORe-communication.   *
 *                                                                           *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                               *
 * 2005-11-15                                                                *
 *****************************************************************************/

#include "ore-local.h"

static int add_dde_region(l4dm_dataspace_t *ds, dsi_socket_t *sock);

static LIST_HEAD(dsi_packet_list);

/* Determine if addr. is within a client dataspace.*/
int __l4ore_in_dataspace(void *addr, dsi_packet_t **packet,
	dsi_socket_t **socket)
{
    struct list_head *p, *h, *n;
    h = &dsi_packet_list;

    if (!addr)
    {
        *packet = NULL;
        *socket = NULL;
        LOGd(ORE_DEBUG_DSI,"invalid address");
        return 0;
    }

    list_for_each_safe(p,n,h)
    {
        ore_dsi_desc *desc = list_entry(p, ore_dsi_desc, list);
        if (desc->addr <= addr && (desc->addr + desc->size) > addr)
        {
            *packet = desc->packet;
            *socket = desc->socket;
            list_del(p);
            kfree(desc);
            return 1;
        }
    }

    return 0;
}

/* commit a packet within a DSI dataspace */
void __l4ore_do_packet_commit(dsi_packet_t *pack, dsi_socket_t *sock)
{
	if (pack && sock)
	{
		int ret = dsi_packet_commit(sock, pack);
		LOGd(ORE_DEBUG_DSI, "commit: %d", ret);
	}
}

/* add packet to packet list */
void __l4ore_remember_packet(dsi_packet_t *pack, dsi_socket_t *sock,
	void *addr, l4_size_t size)
{
    ore_dsi_desc *d = kmalloc(sizeof(ore_dsi_desc), GFP_KERNEL);
    if (d)
    {
        d->addr	  = addr;
        d->size	  = size;
        d->packet = pack;
        d->socket = sock;
        list_add_tail(&d->list, &dsi_packet_list);
    }
    else
        panic("No memory for ore_dsi_desc!");
        
    LOGd(ORE_DEBUG_DSI, "packet semaphores are %d %d",
        pack->rx_sem, pack->tx_sem);
}

/* initialize DSI socket to receive packets from DSI client */
int init_dsi_sendingclient(int channel, l4ore_config *conf)
{
    dsi_jcp_stream_t jcp;
    dsi_stream_cfg_t cfg;
    l4_threadid_t sync = L4_INVALID_ID;
    int ret = 0;
    int flags = DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK;

    LOGd_Enter(ORE_DEBUG_DSI);

    cfg.num_packets = 100;
    cfg.max_sg      = 2;

    if (ORE_DEBUG_DSI)
    {
        LOG_SOCKETREF(&conf->ro_send_client_socketref);
    }

    ret = dsi_socket_create(jcp, cfg, &conf->ro_send_ctl_ds,
            &conf->ro_send_ds, ore_connection_table[channel].worker_dsi,
            &sync, flags, &ore_connection_table[channel].tx_socket);
    LOGd(ORE_DEBUG_DSI, "socket_create: %d", ret);
    if (ret)
    {
        LOG_Error("dsi_socket_create failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    ret = dsi_socket_get_ref(ore_connection_table[channel].tx_socket,
            &conf->ro_send_ore_socketref);
    LOGd(ORE_DEBUG_DSI, "get socketref: %d", ret);
    if (ret)
    {
        LOG_Error("dsi_socket_get_ref failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    ret = dsi_socket_connect(ore_connection_table[channel].tx_socket,
            &conf->ro_send_client_socketref);
    LOGd(ORE_DEBUG_DSI, "connect: %d", ret);
    if (ret)
    {
        LOG_Error("error on socket connect: %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    ret = add_dde_region(&conf->ro_send_ds,
                         ore_connection_table[channel].tx_socket);

    return 0;
}

/* initialize DSI socket to send packets to a DSI client */
int init_dsi_receivingclient(int channel, l4ore_config *conf)
{
    dsi_jcp_stream_t jcp;
    dsi_stream_cfg_t cfg;
    l4_threadid_t sync = L4_INVALID_ID;
    int ret = 0;
    
    // the send socket must not block - we want to drop packets if there
    // is no space anymore
    int flags = DSI_SOCKET_SEND;

    LOGd_Enter(ORE_DEBUG_DSI);

    cfg.num_packets = 100;
    cfg.max_sg      = 2;

    if (ORE_DEBUG_DSI)
    {
        LOG_SOCKETREF(&conf->ro_recv_client_socketref);
    }

    ret = dsi_socket_create(jcp, cfg, &conf->ro_recv_ctl_ds,
            &conf->ro_recv_ds, l4_myself(),
            &sync, flags, &ore_connection_table[channel].rx_socket);
    LOGd(ORE_DEBUG_DSI, "socket_create: %d", ret);
    if (ret)
    {
        LOG_Error("dsi_socket_create failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    ret = dsi_socket_get_ref(ore_connection_table[channel].rx_socket,
            &conf->ro_recv_ore_socketref);
    LOGd(ORE_DEBUG_DSI, "get socketref: %d", ret);
    if (ret)
    {
        LOG_Error("dsi_socket_get_ref failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }
    
    ret = dsi_socket_get_data_area(ore_connection_table[channel].rx_socket,
        &ore_connection_table[channel].rx_addr, 
        &ore_connection_table[channel].rx_size);
    LOGd(ORE_DEBUG_DSI, "get data area: %p", ore_connection_table[channel].rx_addr);
    if (ret)
    {
        LOG_Error("dsi_socket_get_data_area failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }
    ore_connection_table[channel].rx_start = ore_connection_table[channel].rx_addr;

    ret = dsi_socket_connect(ore_connection_table[channel].rx_socket,
            &conf->ro_recv_client_socketref);
    LOGd(ORE_DEBUG_DSI, "connect: %d", ret);
    if (ret)
    {
        LOG_Error("error on socket connect: %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    return 0;
}

/* Add region to DDE's physical memory management. This is necessary,
 * because dataspaces are not managed by the DDE and we need to make
 * this areas known, so that the device drivers may use dem for DMA. */
static int add_dde_region(l4dm_dataspace_t *ds, dsi_socket_t *sock)
{
    l4_addr_t phys_addr;
    void *virt_addr;
    l4_size_t phys_size, virt_size;
    int ret;

    LOGd_Enter(ORE_DEBUG_DSI);

    if (!l4dm_mem_ds_is_contiguous(ds))
    {
        LOG_Error("dataspace to be used for rx/tx is not physically contiguous");
        return -L4_EBADF;
    }

    ret = l4dm_mem_ds_phys_addr(ds, 0, L4DM_WHOLE_DS,
        &phys_addr, &phys_size);
    if (ret)
    {
        LOG_Error("error getting physical address of dataspace.");
        return ret;
    }

    ret = dsi_socket_get_data_area(sock, &virt_addr, &virt_size);
    if (ret)
    {
        LOG_Error("error getting socket data area");
        return ret;
    }

    LOGd(ORE_DEBUG_DSI, "adding region to DDE memory:");
    LOGd(ORE_DEBUG_DSI, "virt address = %p", virt_addr);
    LOGd(ORE_DEBUG_DSI, "phys address = %p", (void *)phys_addr);
    l4dde_add_region((l4_addr_t)virt_addr, phys_addr, phys_size);

    return 0;
}
