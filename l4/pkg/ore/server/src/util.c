/****************************************************************
 * ORe utility functions.                                       *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-09-05                                                   *
 ****************************************************************/

#include "ore-local.h"
#include <linux/etherdevice.h>

static ore_mac broadcast_mac = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/******************************************************************************
 * Determine whether two MAC addresses are equal.
 ******************************************************************************/
int mac_equal(ore_mac mac1, ore_mac mac2)
{
  return (memcmp(mac1, mac2, 6) == 0);
}

int mac_is_broadcast(ore_mac mac)
{
  return (memcmp(mac, broadcast_mac, 6) == 0);
}

/******************************************************************************
 * Find a channel belonging to a specific MAC. As there may be several channels
 * with the same MAC, users may specify a start value from which line to search
 * the connection_table.
 ******************************************************************************/
l4ore_handle_t find_channel_for_skb(struct sk_buff *skb, int start)
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

/* Find channel for mac address. This is used, when we do not have a
 * skb to get information from.
 */
l4ore_handle_t find_channel_for_mac(ore_mac mac)
{
  int i = 0;

  for (i = 0; i < ORE_CONFIG_MAX_CONNECTIONS; i++)
    {
      // skip unused connections
      if (ore_connection_table[i].in_use == 0)
        continue;
      // check for equality
      if (mac_equal(ore_connection_table[i].mac, mac))
        return i;
      // check broadcast
      if (mac_equal(mac, broadcast_mac)
          && ore_connection_table[i].config.rw_broadcast)
        return i;
    }

  return -1;
}

int local_deliver(rxtx_entry_t *ent)
{
  struct sk_buff *new_buf;
  // ent is a SEND entry (== a raw packet). Therefore we cannot
  // use find_channel_for_skb() here.
  int i = find_channel_for_mac(ent->buf->data);

  LOGd(ORE_DEBUG_COMPONENTS, "Trying local delivery...");

  if (i < 0)
    return -1;

  LOGd(ORE_DEBUG_COMPONENTS, "Local delivery necessary.");
  new_buf = skb_clone(ent->buf, GFP_KERNEL);

  // build an rx skb
  // --> type_trans will only be performed once
  new_buf->protocol = eth_type_trans(new_buf, ore_connection_table[i].dev);

  // call the channels' netif_rx() to deliver packets
  do
    {
      new_buf->dev = ore_connection_table[i].dev;
      ore_connection_table[i].netif_rx_func(i, new_buf);
      i = find_channel_for_skb(new_buf, i + 1);
    }
  while (i >= 0);

  kfree_skb(new_buf);

  return 0;
}
