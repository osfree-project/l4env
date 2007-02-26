/****************************************************************
 * ORe interrupt handling.                                      *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include "ore-local.h"

// TODO: hmmm...
void irq_handler(l4_int32_t irq, void *arg)
{
  //LOGd(ORE_DEBUG_IRQ, "irq alien handler: IRQ %d, %p", irq, arg);
}

/* Main netif_rx function. It is called by the device driver and 
 * cares for delivering incoming packets to the clients' own netif_rx()
 * routines.
 */
int netif_rx(struct sk_buff *skb)
{
  // find out who will receive this skb
  int channel = find_channel_for_skb(skb, 0);

  if (ORE_DEBUG_PACKET)
      LOG_SKB(skb);

  // if the first channel lookup fails, the packet is not for us
  if (channel < 0)
    {
      kfree_skb(skb);
      return NET_RX_SUCCESS;
    }

  // driver pulled eth_header out of data area. we push it back,
  // so that we can hand out full packets up to our clients.
  skb_push(skb, skb->dev->hard_header_len);

  while (channel >= 0 && channel < ORE_CONFIG_MAX_CONNECTIONS)
    {
      int ret = NET_RX_SUCCESS;

      // only accept packet if connection is in active state
      if (ore_connection_table[channel].config.rw_active)
      {
        LOGd(ORE_DEBUG_IRQ, "packet for channel %d", channel);
        ret = ore_connection_table[channel].netif_rx_func(channel, skb);
      }

      if (ret != NET_RX_SUCCESS)
        LOG_Error("Error receiving packet for channel %d", channel);

      // find next recipient
      channel = find_channel_for_skb(skb, channel+1);
    }

  // decrement usage count as the irq thread does not use this skb anymore
  atomic_dec(&skb->users);

  return NET_RX_SUCCESS;
}
