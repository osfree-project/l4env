/****************************************************************
 * ORe interrupt handling.                                      *
 *                                                              *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                  *
 * 2005-08-10                                                   *
 ****************************************************************/

#include "ore-local.h"

// ??? What do we need this for ???
void custom_irq_handler(l4_threadid_t client, l4_umword_t dw0, l4_umword_t dw1)
{
  //LOG("custom irq handler: "l4util_idfmt" dw0 = %d, dw1 = %d",
  //    l4util_idstr(client), dw0, dw1);
}

void irq_handler(l4_int32_t irq, void *arg)
{
  LOGd(ORE_DEBUG_IRQ, "irq alien handler: IRQ %d", irq);
}

int netif_rx(struct sk_buff *skb)
{
  l4ore_handle_t channel;

  // find out who will receive this skb
  channel = find_channel_for_skb(skb, 0);

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

      l4lock_lock(&ore_connection_table[channel].channel_lock);

      // discard packet if it is for an inactive connection
      if (ore_connection_table[channel].config.rw_active)
        ret = ore_connection_table[channel].netif_rx_func(channel, skb);

      l4lock_unlock(&ore_connection_table[channel].channel_lock);

      if (ret != NET_RX_SUCCESS)
        LOG_Error("Error sending packet for channel %d", channel);

      // find next recipient
      channel = find_channel_for_skb(skb, channel+1);
    }

  // decrement usage count as the irq thread does not use this skb anymore
  atomic_dec(&skb->users);

  return NET_RX_SUCCESS;
}
