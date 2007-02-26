/* OSHKOSH server stub */

/* L4 */
#include <l4/env/errno.h>
#include <l4/oshkosh/beapi.h>
#include <l4/dde_linux/dde.h>
#include <l4/crtx/ctor.h>

/* local */
#include "liblinux_proc.h"
#include "liblinux_util.h"
#include "liblinux_dev.h"

/* Linux */
#include <linux/etherdevice.h>

struct oshk_stub_data {
	struct net_device_stats stats;

	/* local */
	int id;                     /* oshkosh connection id */
};

/** OSHKOSH DRIVER STUB: dev->open */
static int oshk_stub_open(struct net_device *dev)
{
	LOG_Enter();
	return 0;
}

/** OSHKOSH DRIVER STUB: dev->stop */
static int oshk_stub_stop(struct net_device *dev)
{
	LOG_Enter();
	return 0;
}

/** OSHKOSH DRIVER STUB: dev->hard_start_xmit */
static int oshk_stub_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct oshk_stub_data *lp = (struct oshk_stub_data *)dev->priv;
	int err;
	int id = lp->id;
	oshk_client_descr_t desc;

	err =
		oshk_client_get_send_desc(id, /*ETH_FRAME_LEN */ skb->len, &desc);
	if (err) {
		printk(KERN_ERR "oshk_client_get_send_desc: %s\n",
		         l4env_strerror(-err));
		return 1;
	}

	/* XXX copy skb */
	memcpy(desc.addr, skb->data, skb->len);

	err = oshk_client_send_desc(id, &desc);
	if (err) {
		printk(KERN_ERR "oshk_client_send_desc: %s\n",
		         l4env_strerror(-err));
		return 1;
	}

	lp->stats.tx_bytes += skb->len;
	lp->stats.tx_packets++;
	dev->trans_start = jiffies;

	/* XXX conditionally XXX print some skb stats */
//	liblinux_skb_stats(skb, "tx");
	dev_kfree_skb(skb);

	return 0;
}

/** OSHKOSH DRIVER STUB: dev->get_stats */
static struct net_device_stats *oshk_stub_stats(struct net_device *dev)
{
	struct oshk_stub_data *lp = (struct oshk_stub_data *)dev->priv;

#if 0
	/* If we are connected, get the stats from there. */
	if (priv->name && priv->rcv_socket) {
		err = oshk_nic_get_stats(priv->name,
		                           &priv->rcv_stream->sender.socketref,
		                           &priv->tmpstats);
		if (err == 0) {
			priv->tmpstats.dev.rx_dropped += priv->stats.rx_dropped;
			priv->tmpstats.dev.tx_dropped += priv->stats.tx_dropped;
			return (struct net_device_stats *)&priv->tmpstats.dev;
		}
	}
#endif

	/* XXX L4Linux stub uses oshk_nic_get_stat() if possible */
	return &lp->stats;
}


/** OSHKOSH DRIVER STUB: dev->do_ioctl */
static int oshk_stub_ioctl(struct net_device *dev, struct ifreq *rq,
                           int cmd)
{
	return -EOPNOTSUPP;
}

/** OSHKOSH DRIVER STUB: INTERRUPT AKA RECEIVE */
static void oshk_stub_interrupt(struct net_device *dev)
{
	int err;
	struct oshk_stub_data *lp = (struct oshk_stub_data *)dev->priv;
	int id = lp->id;
	oshk_client_descr_t desc;

	l4dde_process_add_worker();

	/* we're up - notify creator */
	l4thread_started(NULL);

	while (13) {
		struct sk_buff *skb;
		unsigned size;

		if ((err = oshk_client_poll_desc(id, &desc)) != 0) {
			printk("oshk_client_poll_desc: %s -- Retry?!\n",
			          l4env_strerror(-err));
			l4thread_sleep(1000);
			continue;
		}

		size = desc.size;
		skb = dev_alloc_skb(size + 2);
		if (skb == NULL) {
			printk(KERN_NOTICE "%s: Memory squeeze, dropping packet.\n",
			          dev->name);
			lp->stats.rx_dropped++;
		}
		skb_reserve(skb, 2);    /* 16 byte align the IP header */
		skb->dev = dev;

		/* 'skb->data' points to the start of sk_buff data area. */
		memcpy(skb_put(skb, size), (void *)desc.addr, size);
		skb->protocol = eth_type_trans(skb, dev);

		/* XXX conditionally XXX print some skb stats */
//		liblinux_skb_stats(skb, "rx");
		err = netif_rx(skb);
		/* XXX conditionally XXX check netif_rx() return value */
		liblinux_check_netif_rx(dev, err);
		dev->last_rx = jiffies;
		lp->stats.rx_packets++;
		lp->stats.rx_bytes += size;

		oshk_client_free_desc(id, &desc);
	}
}

/** OSHKOSH DRIVER STUB: STARTUP */
static int oshk_stub_probe(struct net_device *dev)
{
	int err, id;
	l4thread_t th;

	/* Allocate a new 'dev' if needed. */
	if (dev == NULL) {
		/*
		 * Don't allocate the private data here, it is done later
		 * This makes it easier to free the memory when this driver
		 * is used as a module.
		 */
		dev = init_etherdev(0, 0);
		if (dev == NULL)
			return -ENOMEM;
	}

	/* Fill in the 'dev' fields. */
	dev->base_addr = 0;
	dev->open = oshk_stub_open;
	dev->stop = oshk_stub_stop;
	dev->hard_start_xmit = oshk_stub_xmit;
	dev->get_stats = oshk_stub_stats;
	dev->do_ioctl = oshk_stub_ioctl;
	// dev->set_multicast_list = &set_multicast_list;
	dev->irq = -1;
	dev->dma = 0;

	/* Fill in the fields of the device structure with ethernet values. */
	ether_setup(dev);

	/* XXX maybe we need more private data here later */
	dev->priv = kmalloc(sizeof(struct oshk_stub_data), GFP_KERNEL);
	if (dev->priv == NULL)
		return -ENOMEM;
	memset(dev->priv, 0, sizeof(struct oshk_stub_data));

	/* contact oshkosh server */
	id = oshk_client_open(dev->name, &dev->dev_addr,
	                       sizeof(dev->dev_addr));
	if (id < 0) {
		printk("oshk_client_open: %s\n", l4env_strerror(-id));
		return -ENODEV;
	}
	((struct oshk_stub_data *)dev->priv)->id = id;

	err =
		oshk_client_setsockopt(id, DSI_SOCKET_RECEIVE, DSI_SOCKET_BLOCK,
	                         1);
	if (err) {
		printk("oshk_client_setsockopt(RECEIVE, BLOCK): %s\n",
		         l4env_strerror(-err));
		return -ENODEV;
	}
	err = oshk_client_setsockopt(id, DSI_SOCKET_SEND, DSI_SOCKET_BLOCK, 1);
	if (err) {
		printk("oshk_client_setsockopt(SEND, BLOCK): %s\n",
		         l4env_strerror(-err));
		return -ENODEV;
	}

	th = l4thread_create((l4thread_fn_t) oshk_stub_interrupt, dev,
	                      L4THREAD_CREATE_SYNC);
	if (th < 0) {
		oshk_client_close(id);
		printk("l4thread_create: %s\n", l4env_strerror(-th));
		return -ENODEV;
	}

	return 0;
}

static struct net_device this_device = { init:oshk_stub_probe };

static void liblinux_oshkosh_init(void)
{
//	int err = 0;

	printk("OSHK-INIT\n");

	/* Copy the parameters from insmod into the device structure. */
	this_device.base_addr = 0;
	this_device.irq = 0;
	this_device.dma = 0;
	this_device.mem_start = 0;

	/*err =*/ register_netdev(&this_device);
}

L4C_CTOR(liblinux_oshkosh_init, L4CTOR_FLIPS_OSHKOSH);
