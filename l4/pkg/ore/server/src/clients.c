/****************************************************************
 * ORe client handling functions.                               *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include "ore-local.h"

static int device_mac_available = 1;

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
    }
}

/******************************************************************************
 * Get the first unused channel from the connection_table.
 ******************************************************************************/
l4ore_handle_t getUnusedConnection(void)
{
  int i;

  for (i = 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
      l4lock_lock(&ore_connection_table[i].channel_lock);

      if (ore_connection_table[i].in_use == 0)
        // DO NOT RELEASE THE LOCK!
        break;

      l4lock_unlock(&ore_connection_table[i].channel_lock);
    }

  if (i >= ORE_CONFIG_MAX_CONNECTIONS)
    return (l4ore_handle_t)-L4_EMFILE;

  // mark the connection "in use"
  ore_connection_table[i].in_use = 1;

  l4lock_unlock(&ore_connection_table[i].channel_lock);

  return i;
}

/******************************************************************************
 * Initialize a client connection.
 ******************************************************************************/
int setup_connection(char *device_name, ore_mac mac,
                     const l4dm_dataspace_t *send_ds,
                     const l4dm_dataspace_t *recv_ds,
                     unsigned char mac_address_head[4],
                     l4ore_config *conf, l4ore_handle_t handle)
{
  // check if we have this device name
  ore_connection_table[handle].dev = dev_get_by_name(device_name);
  if (ore_connection_table[handle].dev == NULL)
    {
      free_connection(handle);
      return -ENODEV;
    }

  // make sure the device mac is only handed out to the first client
  // requesting it
  if (conf->ro_keep_device_mac)
    {
      if (device_mac_available)
	  {
	    LOG("Allocating physical MAC address to client.");
	    memcpy(mac, ore_connection_table[handle].dev->dev_addr, 6);
	    device_mac_available = 0;
	  }
      else
	  {
	    LOG("Physcial MAC address not available!");
            conf->ro_keep_device_mac = 0;
	    goto copy;
	  }
    }
  else
    {
    copy:
      mac[0] = mac_address_head[0];
      mac[1] = mac_address_head[1];
      mac[2] = mac_address_head[2];
      mac[3] = mac_address_head[3];
      mac[4] = 0x00;
      mac[5] = handle;
    }

  memcpy(ore_connection_table[handle].mac, mac, 6);

  // TODO: for shared memory connections it will be useful when
  // we open another connection for the same MAC address and the
  // same client that we make sure that both connections use the
  // same send and receive data spaces.

  if ((recv_ds != NULL)
      && !l4dm_dataspace_equal(*recv_ds, L4DM_INVALID_DATASPACE))
    {
      ore_connection_table[handle].recv_ds           = *recv_ds;
      ore_connection_table[handle].rx_component_func = NULL;
      ore_connection_table[handle].rx_reply_func     = NULL;
      ore_connection_table[handle].netif_rx_func     = NULL;
    }
  else
    {
      ore_connection_table[handle].recv_ds           = L4DM_INVALID_DATASPACE;
      INIT_LIST_HEAD(&ore_connection_table[handle].rx_list);
      ore_connection_table[handle].rx_component_func = rx_component_string;
      ore_connection_table[handle].rx_reply_func     = rx_to_client_string;
      ore_connection_table[handle].netif_rx_func     = netif_rx_string;
    }

  if ((send_ds != NULL)
      && !l4dm_dataspace_equal(*send_ds, L4DM_INVALID_DATASPACE))
    {
      ore_connection_table[handle].send_ds           = *send_ds;
      ore_connection_table[handle].tx_component_func = NULL;
    }
  else
    {
      ore_connection_table[handle].send_ds           = L4DM_INVALID_DATASPACE;
      INIT_LIST_HEAD(&ore_connection_table[handle].tx_list);
      ore_connection_table[handle].tx_component_func = tx_component_string;
    }

  ore_connection_table[handle].flags          = 0;
  // configuration, add the read-only values
  ore_connection_table[handle].config         = *conf;
  ore_connection_table[handle].config.ro_irq  = ore_connection_table[handle].dev->irq;
  ore_connection_table[handle].config.ro_mtu  = ore_connection_table[handle].dev->mtu;
  *conf = ore_connection_table[handle].config;
  ore_connection_table[handle].channel_lock   = L4LOCK_UNLOCKED;
  ore_connection_table[handle].waiting_client = L4_INVALID_ID;
  ore_connection_table[handle].waiting_size   = 0;

  return 0;
}

/******************************************************************************
 * Perform everything necessary to free an rxtx_entry.
 ******************************************************************************/
void free_rxtx_entry(rxtx_entry_t *e)
{
  // don't let the evil users fool us...
  if (e == NULL)
    return;

  // if there is an skb inside, call kfree_skb
  if (e->buf)
    kfree_skb(e->buf);

  // the skb may be freed now _OR_ kfree_skb only decremented the usage
  // counter --> free e only if the belonging skb was freed. Otherwise the
  // entry is still located within another rx_list and must not be deleted.
  if (!e->buf || atomic_read(&e->buf->users) < 2)
    kfree(e);
}

/******************************************************************************
 * Clear a list of rxtx entries.
 ******************************************************************************/
void clear_rxtx_list(struct list_head *h)
{
  struct list_head *p, *n;

  list_for_each_safe(p, n, h)
    {
      rxtx_entry_t *entry = list_entry(p, rxtx_entry_t, list);
      free_rxtx_entry(entry);
      list_del(p);
    }
}

/******************************************************************************
 * Cleanup connection state.
 ******************************************************************************/
int free_connection(l4ore_handle_t handle)
{
  ore_connection_table[handle].in_use         = 0;

  if (ore_connection_table[handle].dev
      && memcmp(ore_connection_table[handle].mac,
                ore_connection_table[handle].dev->dev_addr, 6) == 0)
    device_mac_available = 1;

  // TODO: probably we should detach the dataspaces here...
  ore_connection_table[handle].recv_ds	      = L4DM_INVALID_DATASPACE;
  ore_connection_table[handle].send_ds        = L4DM_INVALID_DATASPACE;
  ore_connection_table[handle].flags          = 0;
  ore_connection_table[handle].config         = L4ORE_INVALID_CONFIG;
  ore_connection_table[handle].channel_lock   = L4LOCK_UNLOCKED;

  memset(ore_connection_table[handle].mac, 0, 6);
  ore_connection_table[handle].waiting_client = L4_INVALID_ID;
  ore_connection_table[handle].waiting_size   = 0;

  // clear rx list and tx list
  if (ore_connection_table[handle].dev)
    {
      clear_rxtx_list(&ore_connection_table[handle].rx_list);
      clear_rxtx_list(&ore_connection_table[handle].tx_list);
    }

  ore_connection_table[handle].dev            = NULL;

  return 0;
}
