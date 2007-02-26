#include "ore-local.h"

/******************************************************************************
 * Try to deliver packets to local clients.                                   *
 ******************************************************************************/
int local_deliver(rxtx_entry_t *ent)
{
  struct sk_buff *new_buf;
  // ent is a SEND entry (== a raw packet). Therefore we cannot
  // use find_channel_for_skb() here.
  int i = find_channel_for_mac(ent->skb->data, 0);

  LOGd(ORE_DEBUG_PACKET, "Trying local delivery...");

  if (i < 0)
    return -1;

  LOGd(ORE_DEBUG_PACKET, "Local delivery necessary.");
  new_buf = skb_clone(ent->skb, GFP_KERNEL);

  // call the channels' netif_rx() to deliver packets
  do
    {
      new_buf->dev = ore_connection_table[i].dev;
      ore_connection_table[i].netif_rx_func(i, new_buf);
      i = find_channel_for_mac(new_buf->data, i + 1);
    }
  while (i >= 0);

  kfree_skb(new_buf);

  return 0;
}
