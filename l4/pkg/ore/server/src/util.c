/****************************************************************
 * ORe utility functions.                                       *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-09-05                                                   *
 *                                                              *
 * (c) 2005 - 2007 Technische Universitaet Dresden				*
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "ore-local.h"

static ore_mac broadcast_mac = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/******************************************************************************
 * Determine whether two MAC addresses are equal.
 ******************************************************************************/
int mac_equal(ore_mac mac1, ore_mac mac2)
{
  return (mac1 && mac2 && memcmp(mac1, mac2, 6) == 0);
}

/******************************************************************************
 * Determine whether a MAC is a broadcast address.
 ******************************************************************************/
int mac_is_broadcast(ore_mac mac)
{
  return (mac && memcmp(mac, broadcast_mac, 6) == 0);
}

/******************************************************************************
 * Find the channel number belonging to a specific thread id.
 ******************************************************************************/
int find_channel_for_threadid(l4_threadid_t t, int start)
{
  int i;

  for (i = (start >= 0) ? start : 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
      // skip unused connections
      if (ore_connection_table[i].in_use == 0)
        continue;
    
      if (l4_task_equal(ore_connection_table[i].owner, t))
        return i;       
    }

  return -1;
}

/******************************************************************************
 * Find a channel belonging to a specific MAC. As there may be several channels
 * with the same MAC, users may specify a start value from which line to search
 * the connection_table.
 ******************************************************************************/
int find_channel_for_skb(struct sk_buff *skb, int start)
{
  int i;

  for (i = (start > 0) ? start : 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
      // skip unused connections
      if (ore_connection_table[i].in_use == 0)
        continue;

      // skip if the devices don't match
      if (skb->dev != ore_connection_table[i].dev)
        continue;

      // check for equality
      if (mac_equal(ore_connection_table[i].mac, skb->mac.raw))
        return i;

      // check for broadcast
      if (mac_equal(skb->mac.raw, broadcast_mac)
          && ore_connection_table[i].config.rw_broadcast)
        return i;
    }

  return -1;
}

/******************************************************************************
 * Find the channel number for a specific MAC address.                        *
 ******************************************************************************/
int find_channel_for_mac(ore_mac mac, int start)
{
  int i = 0;

  for (i = (start > 0) ? start : 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
      // skip unused connections
      if (ore_connection_table[i].in_use == 0)
        continue;
      // check for equality
      if (mac_equal(ore_connection_table[i].mac, mac))
        return i;
      // check broadcast
      if (ore_connection_table[i].config.rw_broadcast && 
	      mac_equal(mac, broadcast_mac))
        return i;
    }

  return -1;
}

/******************************************************************************
 * Find the channel served by a worker thread.                                *
 ******************************************************************************/
int find_channel_for_worker(l4ore_handle_t worker)
{
    int i=0;
    for (i=0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
        if (!ore_connection_table[i].in_use)
            continue;
        if (l4_thread_equal(ore_connection_table[i].worker, worker))
            return i;
    }

    return -1;
}

/******************************************************************************
 * Perform sanity checks for send and receive:                                *
 *  - check ownership of connection                                           *
 *  - check if connection is marked "in use"                                  *
 ******************************************************************************/
int sanity_check_rxtx(int handle, l4_threadid_t client)
{
  if (!l4_task_equal(ore_connection_table[handle].owner, client))
    {
      LOG_Error(l4util_idfmt" is not owner of connection %d", 
              l4util_idstr(client), handle);
      return -L4_EBADF;
    }
  
  if (ore_connection_table[handle].in_use == 0)
    {
      LOG_Error("Trying to send via unused connection.");
      return -L4_EBADF;
    }

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
    if (e->skb)
        kfree_skb(e->skb);

#if 0
    // the skb may be freed now _OR_ kfree_skb only decremented the usage
    // counter --> free e only if the belonging skb was freed. Otherwise the
    // entry is still located within another rx_list and must not be deleted.
    if (!e->skb || atomic_read(&e->skb->users) < 2)
#endif
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
        list_del(p);
        free_rxtx_entry(entry);
    }
}
