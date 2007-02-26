/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "local.h"
#include "list.h"

static LIST_HEAD(dsi_packet_list);

typedef struct
{
    struct list_head    list;
    void                *addr;
    l4_size_t           size;
    dsi_packet_t        *pack;
    dsi_socket_t        *sock;
} ore_dsi_desc;

void __l4ore_remember_packet(dsi_socket_t *sock, dsi_packet_t *pack, void *addr, l4_size_t size)
{
    ore_dsi_desc *d = malloc(sizeof(ore_dsi_desc));

    if (!d)
        return;
            
    d->addr = addr;
    d->size = size;
    d->pack = pack;
    d->sock = sock;

    list_add_tail(&d->list, &dsi_packet_list);
}

void l4ore_packet_to_pool(void *addr)
{
    struct list_head *p, *h, *n;
    h = &dsi_packet_list;

    if (!addr)
        return;

    list_for_each_safe(p,n,h)
    {
        ore_dsi_desc *d = list_entry(p, ore_dsi_desc, list);
        if (d->addr <= addr && (d->addr + d->size) > addr)
        {
            dsi_packet_commit(d->sock, d->pack);
            list_del(p);
            free(d);
            return;
        }
    }
}

/* create DSI connection for sending data */
int __l4ore_init_send_socket(l4ore_handle_t server, l4ore_config *conf, dsi_socket_t **sock, void **addr)
{
    dsi_jcp_stream_t jcp;
    dsi_stream_cfg_t cfg;
    int flags, ret = 0;
    l4_threadid_t worker    = l4_myself();
    l4_threadid_t sync      = L4_INVALID_ID;
    int size, a;

    LOG_Enter();
    
    // send socket, 100 packets
    flags                   = DSI_SOCKET_SEND | DSI_SOCKET_BLOCK;
    cfg.num_packets         = 100;
    cfg.max_sg              = 2;
    conf->ro_send_ctl_ds    = L4DM_INVALID_DATASPACE;
    
    // create socket, let DSI create control ds and sync thread
    ret = dsi_socket_create(jcp, cfg, &conf->ro_send_ctl_ds,
            &conf->ro_send_ds, worker, &sync, flags, sock);
    LOG("socket_create: %d", ret);
    if (ret)
    {
        LOG("dsi_socket_create failed: %d (%s)", ret, l4env_strerror(-ret));
        return ret;
    }
    
    // get DSI socket reference
    ret = dsi_socket_get_ref(*sock, &conf->ro_send_client_socketref);
    LOG("socket_get_ref: %d", ret);
    if (ret)
    {
        LOG("dsi_socket_get_ref failed: %d (%s)", ret, l4env_strerror(-ret));
        return ret;
    }

    ret = dsi_socket_get_data_area(*sock, addr, &size);
    LOG("got data area of size %d: %p", size, *addr);
    if (ret)
    {
        LOG("get_data_area failed: %d", ret);
        return ret;
    }

    a = 0;
    while (a < size)
    {
        *((int*)(*addr+a)) = 0;
        a += L4_PAGESIZE;
    }

    LOG_SOCKETREF(&conf->ro_send_client_socketref);

	// XXX: broken - need to locally lookup the ORe server here
    // share dataspaces and socket with ORe
    ret = dsi_socket_share_ds(*sock, server);
    LOG("socket_share_ds: %d", ret);
    if (ret)
    {
        LOG("dsi_socket_share failed: %d (%s)", ret, l4env_strerror(-ret));
        return ret;
    }

    ret = l4dm_share(&conf->ro_send_ctl_ds, server, L4DM_RW);
    LOG("share ctl_ds: %d", ret);
    if (ret)
    {
        LOG("sharing ctl ds failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    ret = l4dm_share(&conf->ro_send_ds, server, L4DM_RW);
    LOG("share send_ds: %d", ret);
    if (ret)
    {
        LOG("sharing ctl ds failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    return 0;
}

int __l4ore_init_recv_socket(l4ore_handle_t server, l4ore_config *conf, dsi_socket_t **sock, void **addr)
{
    dsi_jcp_stream_t jcp;
    dsi_stream_cfg_t cfg;
    int flags, ret = 0;
    l4_threadid_t worker    = l4_myself();
    l4_threadid_t sync      = L4_INVALID_ID;
    int size, a;

    LOG_Enter();
    
    // receive socket, 100 packets
    flags                   = DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK;
    cfg.num_packets         = 100;
    cfg.max_sg              = 2;
    conf->ro_recv_ctl_ds    = L4DM_INVALID_DATASPACE;
    
    // create socket, let DSI create control ds and sync thread
    ret = dsi_socket_create(jcp, cfg, &conf->ro_recv_ctl_ds,
            &conf->ro_recv_ds, worker, &sync, flags, sock);
    LOG("socket_create: %d", ret);
    if (ret)
    {
        LOG("dsi_socket_create failed: %d (%s)", ret, l4env_strerror(-ret));
        return ret;
    }
    
    // get DSI socket reference
    ret = dsi_socket_get_ref(*sock, &conf->ro_recv_client_socketref);
    LOG("socket_get_ref: %d", ret);
    if (ret)
    {
        LOG("dsi_socket_get_ref failed: %d (%s)", ret, l4env_strerror(-ret));
        return ret;
    }

    ret = dsi_socket_get_data_area(*sock, addr, &size);
    LOG("got data area of size %d: %p", size, *addr);
    if (ret)
    {
        LOG("get_data_area failed: %d", ret);
        return ret;
    }

    a = 0;
    while (a < size)
    {
        *((int*)(*addr+a)) = 0;
        a += L4_PAGESIZE;
    }

    LOG_SOCKETREF(&conf->ro_recv_client_socketref);

    // share dataspaces and socket with ORe
    ret = dsi_socket_share_ds(*sock, server);
    LOG("socket_share_ds: %d", ret);
    if (ret)
    {
        LOG("dsi_socket_share failed: %d (%s)", ret, l4env_strerror(-ret));
        return ret;
    }

    ret = l4dm_share(&conf->ro_recv_ctl_ds, server, L4DM_RW);
    LOG("share ctl_ds: %d", ret);
    if (ret)
    {
        LOG("sharing ctl ds failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    ret = l4dm_share(&conf->ro_recv_ds, server, L4DM_RW);
    LOG("share send_ds: %d", ret);
    if (ret)
    {
        LOG("sharing ctl ds failed. %d (%s)", ret,
                l4env_strerror(-ret));
        return ret;
    }

    return 0;}
