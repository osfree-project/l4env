/** THIS IS FROM LINUX (linux/drivers/net/loopback.c) */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/init.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/if_ether.h>     /* For the statistics structure. */
#include <linux/if_arp.h>       /* For ARPHRD_ETHER */

#include "local.h"

#define LOOPBACK_OVERHEAD (128 + MAX_HEADER + 16 + 16)

/*
 * The higher levels take care of making this non-reentrant (it's
 * called with bh's disabled).
 */
static int loopback_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int err;
	struct net_device_stats *stats = (struct net_device_stats *)dev->priv;

	/*
	 *    Optimise so buffers with skb->free=1 are not copied but
	 *    instead are lobbed from tx queue to rx queue 
	 */

	if (atomic_read(&skb->users) != 1) {
		struct sk_buff *skb2 = skb;
		skb = skb_clone(skb, GFP_ATOMIC);   /* Clone the buffer */
		if (skb == NULL) {
			LOG("skb_clone() returned NULL");
			kfree_skb(skb2);
			return 0;
		}
		kfree_skb(skb2);
	} else
		skb_orphan(skb);

	skb->protocol = eth_type_trans(skb, dev);
	skb->dev = dev;
#ifndef LOOPBACK_MUST_CHECKSUM
	skb->ip_summed = CHECKSUM_UNNECESSARY;
#endif

	dev->last_rx = jiffies;
	stats->rx_bytes += skb->len;
	stats->tx_bytes += skb->len;
	stats->rx_packets++;
	stats->tx_packets++;

	/* XXX conditionally XXX print some skb stats */
	//liblinux_skb_stats(skb, TX);

	err = netif_rx(skb);

	/* XXX conditionally XXX check netif_rx() return value */
	liblinux_check_netif_rx(dev, err);

	return (0);
}

static struct net_device_stats *get_stats(struct net_device *dev)
{
	return (struct net_device_stats *)dev->priv;
}

/** INTERFACE TO THE OUTER REALMS */

int liblinux_lo_init(struct net_device *dev)
{
	dev->mtu = (16 * 1024) + 20 + 20 + 12;
	dev->hard_start_xmit = loopback_xmit;
	dev->hard_header = eth_header;
	dev->hard_header_cache = eth_header_cache;
	dev->header_cache_update = eth_header_cache_update;
	dev->hard_header_len = ETH_HLEN;    /* 14           */
	dev->addr_len = ETH_ALEN;   /* 6            */
	dev->tx_queue_len = 0;
	dev->type = ARPHRD_LOOPBACK;    /* 0x0001       */
	dev->rebuild_header = eth_rebuild_header;
	dev->flags = IFF_LOOPBACK;
	dev->features =
		NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_NO_CSUM | NETIF_F_HIGHDMA;
	dev->priv = kmalloc(sizeof(struct net_device_stats), GFP_KERNEL);
	if (dev->priv == NULL)
		return -ENOMEM;
	memset(dev->priv, 0, sizeof(struct net_device_stats));
	dev->get_stats = get_stats;

	printk("lo: I'm up now.\n");

	return (0);
};

