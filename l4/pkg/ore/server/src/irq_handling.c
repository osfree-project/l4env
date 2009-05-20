/****************************************************************
 * ORe interrupt handling.                                      *
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

static int __netif_rx(struct sk_buff *);

// TODO: hmmm...
void irq_handler(l4_int32_t irq, void *arg)
{
  //LOGd(ORE_DEBUG_IRQ, "irq alien handler: IRQ %d, %p", irq, arg);
}

/* Main net rx callback. It is called by the device driver and 
 * cares for delivering incoming packets to the clients' own netif_rx()
 * routines.
 */
#ifdef CONFIG_ORE_DDE24
int netif_rx(struct sk_buff *skb)
#else /* DDE2.6 callback mode */
int l4ore_rx_handle(struct sk_buff *skb)
#endif
{
	int ret = __netif_rx(skb);
	return ret;
}

static int __netif_rx(struct sk_buff *skb)
{
	int channel;

	if (skb && skb->dev && (strcmp(skb->dev->name, "lo") == 0))
		return NET_RX_SUCCESS;

	// find out who will receive this skb
	channel = find_channel_for_skb(skb, 0);

	if (ORE_DEBUG_PACKET) {
		LOG_SKB(skb);
		LOG_MAC(ORE_DEBUG_PACKET, skb->mac_header);
	}

	// if the first channel lookup fails, the packet is not for us
	if (channel < 0)
	{
		LOGd(ORE_DEBUG_PACKET, "	this packet is not for me. OVER.");
		kfree_skb(skb);
		return NET_RX_SUCCESS;
	}

		// driver pulled eth_header out of data area. we push it back,
		// so that we can hand out full packets up to our clients.
		skb_push(skb, skb->dev->hard_header_len);

	/* the IRQ thread does no longer own this skb, however we don't
	 * free it, because this is done by the clients afterwards
	 */
	atomic_dec(&skb->users);

	while (channel >= 0 && channel < ORE_CONFIG_MAX_CONNECTIONS) {
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

  return NET_RX_SUCCESS;
}
