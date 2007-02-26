/*
 * Ore network driver stub.
 *
 * Author: Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */

/*
 * FLIPS adaption by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * This drivern was taken from L4Linux 2.6 tree and modified for Linux 2.4
 * based FLIPS server.
 *
 * I removed IRQ request as we just startup a kernel thread for
 * l4ore_recv_blocking(). -- Christian.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <l4/ore/ore.h>

MODULE_AUTHOR("Adam Lackorzynski <adam@os.inf.tu-dresden.de>");
MODULE_AUTHOR("Christian Helmuth <ch12@os.inf.tu-dresden.de>");
MODULE_DESCRIPTION("Ore stub driver");
MODULE_LICENSE("GPL");

static char *l4x_ore_devname = "eth0";

#define MAC_FMT    "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_ARG(x) x->dev_addr[0], x->dev_addr[1], x->dev_addr[2], \
                   x->dev_addr[3], x->dev_addr[4], x->dev_addr[5]

struct l4x_ore_priv {
	struct net_device_stats    net_stats;

	int                        handle;
	l4ore_config               config;
	unsigned char              *pkt_buffer;
	unsigned long              pkt_size;
};

static struct net_device *l4x_ore_dev;

static int l4x_ore_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct l4x_ore_priv *priv = netdev_priv(netdev);

	if (skb->len < ETH_ZLEN) {
		skb = skb_padto(skb, ETH_ZLEN);
		if (skb == NULL)
			return 0;
		skb->len = ETH_ZLEN;
	}

	if (l4ore_send(priv->handle, (char *)skb->data, skb->len) < 0) {
		LOG_printf("%s: send failed\n", netdev->name);
		return 1; /* Hmm, which return type to take? */
	}

	dev_kfree_skb(skb);

	netdev->trans_start = jiffies;
	priv->net_stats.tx_packets++;
	priv->net_stats.tx_bytes += skb->len;

	return 0;
}

static struct net_device_stats *l4x_ore_get_stats(struct net_device *netdev)
{
	struct l4x_ore_priv *priv = netdev_priv(netdev);
	return &priv->net_stats;
}

static void l4x_ore_tx_timeout(struct net_device *netdev)
{
	LOG_printf("%s\n", __func__);
}

/*
 * Interrupt.
 */
static void l4x_ore_interrupt(struct net_device *netdev)
{
	struct l4x_ore_priv *priv = netdev_priv(netdev);
	struct sk_buff *skb;

	skb = dev_alloc_skb(priv->pkt_size);
	if (likely(skb != NULL)) {
		skb->dev = netdev;
		memcpy(skb_put(skb, priv->pkt_size),
		       priv->pkt_buffer, priv->pkt_size);

		skb->protocol = eth_type_trans(skb, netdev);
		netif_rx(skb);

		netdev->last_rx = jiffies;
		priv->net_stats.rx_bytes += skb->len;
		priv->net_stats.rx_packets++;

	} else {
		printk(KERN_WARNING "%s: dropping packet.\n", netdev->name);
		priv->net_stats.rx_dropped++;
	}
}

/*
 * Receive thread to get packets
 */
static int l4x_ore_recv_thread(void *data)
{
	struct net_device *netdev = (struct net_device *)data;
	struct l4x_ore_priv *priv = netdev_priv(netdev);
	int ret;

	while (1) {
		unsigned int size = ETH_FRAME_LEN;
		ret = l4ore_recv_blocking(priv->handle,
		                          (char **)&priv->pkt_buffer, &size,
		                          L4_IPC_NEVER);
		if (unlikely(ret < 0)) {
			LOG_printf("%s: l4ore_recv_blocking failed: %d\n",
			           netdev->name, ret);
			l4_sleep(100);
			continue;
		} else if (unlikely(ret > 0)) {
			LOG_printf("%s: buffer too small (%d)\n", netdev->name, ret);
		}

		priv->pkt_size = size;

		cli();
		l4x_ore_interrupt(netdev);
		sti();
	}

	/* never reached */
	return -EINTR;
}

static int l4x_ore_open(struct net_device *netdev)
{
	struct l4x_ore_priv *priv = netdev_priv(netdev);
	int err = -ENODEV;

	netif_carrier_off(netdev);

	if (!priv->config.rw_active) {
		priv->config.rw_active = 1;
		l4ore_set_config(priv->handle, &priv->config);
	}

	priv->pkt_buffer = kmalloc(ETH_FRAME_LEN, GFP_KERNEL);
	if (!priv->pkt_buffer) {
		printk("%s: kmalloc error\n", netdev->name);
		goto err_out_close;
	}

	int err2;
	/* XXX err2 will be "thread_t id" of the new thread; we could exploit this
	 * for shutdown */
	err2 = kernel_thread(l4x_ore_recv_thread, netdev, 0);
	if (err2 < 0) {
		printk("%s: Cannot create thread\n", netdev->name);
		err = -EBUSY;
		goto err_out_kfree;
	}

	netif_carrier_on(netdev);
	netif_wake_queue(netdev);

	printk("%s: interface up.\n", netdev->name);

	return 0;

err_out_kfree:
	kfree(priv->pkt_buffer);

err_out_close:
	l4ore_close(priv->handle);
	priv->config.rw_active = 0;
	priv->handle           = 0;
	return err;
}

static int l4x_ore_close(struct net_device *netdev)
{
	struct l4x_ore_priv *priv = netdev_priv(netdev);

	netif_stop_queue(netdev);
	netif_carrier_off(netdev);

	kfree(priv->pkt_buffer);

	priv->config.rw_active = 0;

	return 0;
}

static int __init l4x_ore_init(void)
{
	struct l4x_ore_priv *priv;
	struct net_device *dev;
	int err = -ENODEV;

	if (!(dev = alloc_etherdev(sizeof(struct l4x_ore_priv))))
		return -ENOMEM;

	l4x_ore_dev          = dev;
	dev->open            = l4x_ore_open;
	dev->stop            = l4x_ore_close;
	dev->hard_start_xmit = l4x_ore_xmit_frame;
	dev->get_stats       = l4x_ore_get_stats;
	dev->tx_timeout      = l4x_ore_tx_timeout;

	priv = netdev_priv(dev);

	priv->config = L4ORE_DEFAULT_CONFIG;
	priv->config.rw_debug           = 0;
	priv->config.rw_broadcast       = 1;
	priv->config.ro_keep_device_mac = 1;
	priv->config.rw_active          = 0;

	/* XXX Hmm, we need to open the connection here to get the MAC :/ */
	if ((priv->handle = l4ore_open(l4x_ore_devname, dev->dev_addr,
	                               &priv->config)) < 0) {
		printk("%s: l4ore_open failed: %d\n",
		       dev->name, priv->handle);
		goto err_out_free_dev;
	}

	dev->irq = priv->config.ro_irq;
	dev->mtu = priv->config.ro_mtu;

	if ((err = register_netdev(dev))) {
		printk("l4ore: Cannot register net device, aborting.\n");
		goto err_out_free_dev;
	}

	printk(KERN_INFO "%s: L4Ore card found with " MAC_FMT ", IRQ %d\n",
	                 dev->name, MAC_ARG(dev), dev->irq);

	return 0;

err_out_free_dev:
	free_netdev(dev);

	return err;
}

static void __exit l4x_ore_exit(void)
{
	l4ore_close(((struct l4x_ore_priv *)l4x_ore_dev->priv)->handle);
	unregister_netdev(l4x_ore_dev);
	free_netdev(l4x_ore_dev);
}

module_init(l4x_ore_init);
module_exit(l4x_ore_exit);
