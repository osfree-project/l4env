/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "local.h"
#include <stdlib.h>

#include <l4/names/libnames.h>

ore_client_conn_desc descriptor_table[CONN_MAX];
int ore_initialized = 0;

static int get_free_descriptor(void);
static void release_descriptor(int idx);
static int ore_initialize(void);

#define ORE_NUM_LOOKUPS     10

// find the first free connection
static int get_free_descriptor(void)
{
    int i=0;
    do
    {
        if (l4_is_invalid_id(descriptor_table[i].remote_worker_thread))
            return i;
    }
    while (++i < CONN_MAX);

    return -1;
}

// mark connection as free
static void release_descriptor(int idx)
{
    descriptor_table[idx].remote_worker_thread = L4_INVALID_ID;
}

// initialize library
static int ore_initialize(void)
{
    int i=0;

    // free all client descriptors
    for (i=0; i<CONN_MAX; i++)
    {
        release_descriptor(i);
    }

#ifdef ORE_DSI
    LOG("dsi_init: %d", dsi_init());
#endif

    ore_initialized = 1;
    return 0;
}

int l4ore_recv_blocking(int handle, char **buf, l4_size_t *size, l4_timeout_t timeout)
{
  l4ore_handle_t channel = descriptor_table[handle].remote_worker_thread;
  return descriptor_table[handle].rx_func_blocking(channel, handle, buf, size, timeout);
}

int l4ore_recv_nonblocking(int handle, char **buf, l4_size_t *size)
{
  l4ore_handle_t channel = descriptor_table[handle].remote_worker_thread;
  return descriptor_table[handle].rx_func_nonblocking(channel, handle, buf, size);
}

int l4ore_send(int handle, char *buf, l4_size_t size)
{
  l4ore_handle_t channel = descriptor_table[handle].remote_worker_thread;
  return descriptor_table[handle].send_func(channel, handle, buf, size);
}

int l4ore_open(char *device, unsigned char mac[6], l4ore_config *conf)
{

    l4ore_handle_t ret;
    int desc, err = 0;
#ifdef ORE_DSI
    dsi_socket_t *send = NULL;
    dsi_socket_t *receive = NULL;
#endif

    LOG_Enter();

    if (!conf)
    {
        LOG_Error("invalid connection configuration");
        return -L4_EINVAL;
    }

    // singleton: initialize library
    if (!ore_initialized)
         err = ore_initialize();
    if (err)
        return -L4_EINVAL;

    desc = get_free_descriptor();
    LOG("descriptor: %d", desc);

    // return out of memory if no more handles available
    if (desc < 0)
    {
        LOG_Error("out of descriptor memory");
        return -L4_ENOMEM;
    }
  
    // make sure, we have a name for ORe, fallback is always "ORe"
    if (strlen(conf->ro_orename) == 0)
        strncpy(conf->ro_orename, "ORe", sizeof(conf->ro_orename)-1);
    if (ore_lookup_server(conf->ro_orename, &descriptor_table[desc].remote_manager_thread))
    {
        release_descriptor(desc);
        return -1;
    }

#ifdef ORE_DSI
    // sending via string IPC
    if (l4dm_is_invalid_ds(conf->ro_send_ds))
    {
        LOG("sending via string ipc");
        descriptor_table[desc].send_func = ore_send_string;
        descriptor_table[desc].send_addr = NULL;
    }
    else // sending via dataspace
    {
        LOG("sending via dataspace");
        descriptor_table[desc].send_func = ore_send_dsi;
        __l4ore_init_send_socket(descriptor_table[desc].remote_manager_thread,
                                 conf, &send, &descriptor_table[desc].send_addr);
    }

    // receiving via string IPC
    if (l4dm_is_invalid_ds(conf->ro_recv_ds))
    {
        LOG("receiving via string IPC");
        descriptor_table[desc].rx_func_blocking      = ore_recv_string_blocking;
        descriptor_table[desc].rx_func_nonblocking   = ore_recv_string_nonblocking; 
        descriptor_table[desc].recv_addr             = NULL;
    }
    else // receiving via dataspace
    {
        LOG("receiving via dataspace");
        descriptor_table[desc].rx_func_blocking      = ore_recv_dsi_blocking;
        descriptor_table[desc].rx_func_nonblocking   = ore_recv_dsi_nonblocking;
        __l4ore_init_recv_socket(descriptor_table[desc].remote_manager_thread,
                                 conf, &receive, &descriptor_table[desc].recv_addr);
    }

    descriptor_table[desc].local_send_socket    = send;
    descriptor_table[desc].local_recv_socket    = receive;
#else
    descriptor_table[desc].rx_func_blocking = ore_recv_string_blocking;
    descriptor_table[desc].rx_func_nonblocking = ore_recv_string_nonblocking;
    descriptor_table[desc].send_func        = ore_send_string;
#endif

    ret = ore_do_open(desc, device, mac, conf);
    // return reason of failed open()
    if (l4_is_invalid_id(ret))
    {
#ifdef ORE_DSI
        if (l4dm_is_invalid_ds(conf->ro_send_ds) || 
            l4dm_is_invalid_ds(conf->ro_recv_ds))
        {
            LOG_Error("ore_open() returned INVALID_ID");
            release_descriptor(desc);
            return -1;
        }
        else
        {
            LOG("INVALID worker_id. this is ok, because we do rx/tx via DSI and need no worker.");
        }
#else
        LOG_Error("ore_open() returned INVALID_ID");
        return -1;
#endif
    }

#ifdef ORE_DSI
    if (!l4dm_is_invalid_ds(conf->ro_send_ds))
    {
        int iret = dsi_socket_connect(send, &conf->ro_send_ore_socketref);
        if (iret)
        {
            LOG_Error("send_socket socket connect");
            // TODO: kill sockets ?
            return iret;
        }
    }
    if (!l4dm_is_invalid_ds(conf->ro_recv_ds))
    {
        int iret = dsi_socket_connect(receive, &conf->ro_recv_ore_socketref);
        if (iret)
        {
            LOG_Error("recv_socket socket connect");
            // TODO: kill sockets ?
            return iret;
        }
    }
#endif

    descriptor_table[desc].remote_worker_thread = ret;

    return desc;
}

void l4ore_close(int handle)
{
    ore_do_close(handle);
}

int ore_lookup_server(char *orename, l4ore_handle_t *manager)
{
    int i = 0;

    while(i < ORE_NUM_LOOKUPS)
    {
        if (!names_waitfor_name(orename, manager, 10000))
        {
            LOG("Could not find ORe server '%s', %d. attempt.", orename, i + 1);
            i++;
        }
        else
        {
            LOG("ORe server %s = "l4util_idfmt, orename, l4util_idstr(*manager));
            return 0;
        }
    }
    LOG("Could not find ORe server '%s', aborting.", orename);
    return -1;
}

