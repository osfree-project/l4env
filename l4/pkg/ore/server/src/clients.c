/****************************************************************
 * ORe client handling functions.                               *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 *                                                              *
 * (c) 2005 - 2007 Technische Universitaet Dresden				*
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "ore-local.h"

static int device_mac_available = 1;

static void __init_mac(int channel, l4ore_config *conf, ore_mac mac, 
    unsigned char mac_address_head[4]);
static int __init_workers(int channel, l4ore_config *conf);


/******************************************************************************
 * Initialize the ORe connection table. - Only performs the necessary steps.
 ******************************************************************************/
void init_connection_table(void)
{
    int i;

    for (i = 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
        ore_connection_table[i].channel_lock = L4LOCK_UNLOCKED;
        ore_connection_table[i].in_use       = 0;
        ore_connection_table[i].worker       = L4_INVALID_ID;
    }
}

/******************************************************************************
 * Get the first unused channel from the connection_table.
 ******************************************************************************/
int getUnusedConnection(void)
{
    int i;

    for (i = 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
        if (ore_connection_table[i].in_use == 0)
            break;
    }

    if (i >= ORE_CONFIG_MAX_CONNECTIONS)
        return -L4_EMFILE;

    // mark the connection "in use"
    ore_connection_table[i].in_use = 1;

    return i;
}

/******************************************************************************
 * Generate/setup MAC address.
 ******************************************************************************/ 
static void __init_mac(int channel, l4ore_config *conf, ore_mac mac, 
					   unsigned char mac_address_head[4])
{
    unsigned char data[10];
    static unsigned char zero_head[4] = {0,0,0,0};

    /* MAC generation:
     *
     * 1. try to hand out hardware MAC if requested.
     */
    if (conf->ro_keep_device_mac && device_mac_available)
    {
		LOG("Allocating physical MAC address to client.");
		memcpy(mac, ore_connection_table[channel].dev->dev_addr, 6);
		device_mac_available = 0;
		/* Done here. no more processing. */
		goto out;
	}

	if (conf->ro_keep_device_mac)  /* && !device_mac_available */
	{
            LOG("Physical MAC address not available!");
            conf->ro_keep_device_mac = 0;
    }

    /* 2. no MAC head given at cmd line --> use predefined head and a checksum.
     */
    if (memcmp(mac_address_head, zero_head, 4) == 0)
    {
        memcpy(data, ore_connection_table[channel].dev->dev_addr, 6);
        memcpy(&data[6], &channel, sizeof(int));

        mac[0] = 0x04;
        mac[1] = 0xEA;
        *(unsigned int *)&mac[2]     = adler32(data, 10);
    }
    /* user-provided mac address head 
     */
    else
    {
        mac[0] = mac_address_head[0];
        mac[1] = mac_address_head[1];
        mac[2] = mac_address_head[2];
        mac[3] = mac_address_head[3];
        mac[4] = 0x00;
        mac[5] = channel;
    }

out:
    memcpy(ore_connection_table[channel].mac, mac, 6);
	LOG_MAC(1, ore_connection_table[channel].mac);
}

/******************************************************************************
 * Initialize worker threads.
 ******************************************************************************/ 
static int __init_workers(int channel, l4ore_config *conf)
{
    int ret = 0;
    l4thread_t worker_id;
    
    // We need to spawn a string worker if the client wants to send _OR_
    // receive through string IPC. 
    //
    // Note: only a string IPC worker needs to be entered into the
    //       connection table. DSI workers are managed by the DSI lib.
    if (l4dm_is_invalid_ds(conf->ro_send_ds) || 
        l4dm_is_invalid_ds(conf->ro_recv_ds))
    {
        worker_id = l4thread_create(worker_thread_string, (void *)(&channel),
                L4THREAD_CREATE_SYNC);
        // enter worker threadid in connection table
        ore_connection_table[channel].worker = l4thread_l4_id(worker_id);
        LOGd(ORE_DEBUG_COMPONENTS, "spawned worker thread (sIPC): %d", worker_id);
    }
    
#ifdef ORE_DSI
    // We need to spawn a DSI worker ONLY if the client wants to send
    // through DSI.
    if (!l4dm_is_invalid_ds(conf->ro_send_ds))
    {
        worker_id = l4thread_create(worker_thread_dsi, (void *)(&channel),
                L4THREAD_CREATE_SYNC);
        ore_connection_table[channel].worker_dsi = l4thread_l4_id(worker_id);
        LOGd(ORE_DEBUG_COMPONENTS, "spawned worker thread (DSI): %d", worker_id);
    }

    LOGd(ORE_DEBUG_COMPONENTS, "stored worker: "l4util_idfmt,
            l4util_idstr(ore_connection_table[channel].worker));
#endif

    if (!l4dm_dataspace_equal(conf->ro_send_ds, L4DM_INVALID_DATASPACE))
    {
        ore_connection_table[channel].tx_component_func = NULL;
        ret = init_dsi_sendingclient(channel, conf);
        if (ret)
        {
            // kill worker thread on error
            LOG_Error("Error initializing DSI client. Shutting down worker threads.");
            l4thread_shutdown(l4thread_id(ore_connection_table[channel].worker));
            l4thread_shutdown(l4thread_id(ore_connection_table[channel].worker_dsi));
            return ret;
        }
    }
    else
    {
        INIT_LIST_HEAD(&ore_connection_table[channel].tx_list);
        ore_connection_table[channel].tx_component_func = tx_component_string;
    }   
    
    if(!l4dm_is_invalid_ds(conf->ro_recv_ds))
    {
        ore_connection_table[channel].rx_component_func = NULL;
        ore_connection_table[channel].rx_reply_func     = NULL;
        ore_connection_table[channel].netif_rx_func     = netif_rx_dsi;
        ret = init_dsi_receivingclient(channel, conf);    
        if (ret)
        {
            // kill worker threads on error
            LOG_Error("shutting down worker threads.");
            l4thread_shutdown(l4thread_id(ore_connection_table[channel].worker));
            l4thread_shutdown(l4thread_id(ore_connection_table[channel].worker_dsi));
            return ret;
        }
    }
    else
    {
        INIT_LIST_HEAD(&ore_connection_table[channel].rx_list);
        ore_connection_table[channel].rx_component_func = rx_component_string;
        ore_connection_table[channel].rx_reply_func     = rx_to_client_string;
        ore_connection_table[channel].netif_rx_func     = netif_rx_string;
    }
    
    return 0;
}

/******************************************************************************
 * Initialize a client connection.
 ******************************************************************************/
int setup_connection(char *device_name, ore_mac mac,
                     unsigned char mac_address_head[4],
                     l4ore_config *conf, int channel,
                     l4_threadid_t *owner)
{
    LOGd_Enter(ORE_DEBUG);
    LOGd(ORE_DEBUG, "setting up connection for "l4util_idfmt, l4util_idstr(*owner));
	LOGd(ORE_DEBUG, "device name: %s", device_name);
	LOGd(ORE_DEBUG, "channel: %d", channel);

    // check if we have this device name
    ore_connection_table[channel].dev               = dev_get_by_name(device_name);
    if (ore_connection_table[channel].dev == NULL)
    {
        LOG_Error("no device found.");
        free_connection(channel);
        return -ENODEV;
    }

    // give us a MAC
    __init_mac(channel, conf, mac, mac_address_head);

    // init connection table
    ore_connection_table[channel].flags             = 0;
    ore_connection_table[channel].channel_lock      = L4LOCK_UNLOCKED;
    ore_connection_table[channel].waiting_client    = L4_INVALID_ID;
    ore_connection_table[channel].waiting_size      = 0;
    ore_connection_table[channel].owner             = *owner;
    ore_connection_table[channel].packets_received  = 0;
    ore_connection_table[channel].packets_queued    = 0;
    ore_connection_table[channel].packets_sent      = 0;

    // this lock is used to synchronize with the DSI worker
    ore_connection_table[channel].tx_startlock      = L4LOCK_UNLOCKED;
    l4lock_lock(&ore_connection_table[channel].tx_startlock);

    // default value for the worker
    ore_connection_table[channel].worker            = L4_INVALID_ID;
    ore_connection_table[channel].worker_dsi        = L4_INVALID_ID;

    __init_workers(channel, conf);

    // take over configuration, fill in the read-only values
    ore_connection_table[channel].config            = *conf;
    ore_connection_table[channel].config.ro_irq     = ore_connection_table[channel].dev->irq;
    ore_connection_table[channel].config.ro_mtu     = ore_connection_table[channel].dev->mtu;
    *conf = ore_connection_table[channel].config;

    if (ORE_DEBUG_INIT)
        LOG_CONFIG(*conf);

    // startup sync with the dsi worker
    l4lock_unlock(&ore_connection_table[channel].tx_startlock);
    
    return 0;
}

/******************************************************************************
 * Cleanup connection state.
 ******************************************************************************/
int free_connection(int handle)
{
	// Grab the channel lock here so that no one is able to modify the lists
	// while we are cleaning them up. (The lock is freed down there in a rather
	// unconventional way.
	l4lock_lock(&ore_connection_table[handle].channel_lock);

    ore_connection_table[handle].in_use         = 0;

    // clear rx list and tx list
    if (ore_connection_table[handle].dev)
    {
        clear_rxtx_list(&ore_connection_table[handle].rx_list);
        clear_rxtx_list(&ore_connection_table[handle].tx_list);
    }

    if (ore_connection_table[handle].dev
        && memcmp(ore_connection_table[handle].mac,
           ore_connection_table[handle].dev->dev_addr, 6) == 0)
        device_mac_available = 1;

    // TODO: probably we should detach the dataspaces here...
    ore_connection_table[handle].flags          = 0;
    ore_connection_table[handle].config         = L4ORE_INVALID_CONFIG;
    ore_connection_table[handle].channel_lock   = L4LOCK_UNLOCKED;

    memset(ore_connection_table[handle].mac, 0, 6);
    ore_connection_table[handle].waiting_client = L4_INVALID_ID;
    ore_connection_table[handle].waiting_size   = 0;
    ore_connection_table[handle].packets_received     = -1;
    ore_connection_table[handle].packets_queued       = -1;
    ore_connection_table[handle].packets_sent         = -1;

    ore_connection_table[handle].dev            = NULL;

    return 0;
}
