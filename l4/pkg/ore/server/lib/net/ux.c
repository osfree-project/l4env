/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/if.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_tun.h>

#include <l4/lxfuxlibc/lxfuxlc.h>

#include <l4/sys/vhw.h>
#include <l4/sigma0/kip.h>

struct ux_private {
	int fd;
	int prov_pid;
};

static irqreturn_t ux_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device *dev = dev_id;
	struct ux_private *priv = netdev_priv(dev);
	struct sk_buff *skb = dev_alloc_skb(ETH_FRAME_LEN);
	int ret;

	skb->dev = dev;

	lx_errno = 0;
	if ((ret = lx_read(priv->fd, skb->data, ETH_FRAME_LEN)) < 0) {
		if (lx_errno != EAGAIN)
			printk("uxdev: Error reading packet data: %d\n", lx_errno);
		/* else: ping interrupt, ignore */
		dev_kfree_skb(skb);
		return IRQ_HANDLED;
	}
	skb->len = ret;

	/* ACK interrupt */
	lx_kill(priv->prov_pid, SIGUSR1);

	skb->protocol = eth_type_trans(skb, dev);

	netif_rx(skb);

	return IRQ_HANDLED;
}

static int ux_open(struct net_device *dev)
{
	struct ux_private *priv = netdev_priv(dev);
        int err;

	netif_carrier_off(dev);

	if ((err = request_irq(dev->irq, ux_interrupt,
	                       SA_SAMPLE_RANDOM, dev->name,
	                       dev))) {
		printk("%s: request_irq(%d, ...) failed.\n",
		       dev->name, dev->irq);
		return -ENODEV;
	}

	netif_carrier_on(dev);
	netif_wake_queue(dev);

	printk("%s: interface up.\n", dev->name);

	return 0;
}

static int ux_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ux_private *priv = netdev_priv(dev);

	if (lx_write(priv->fd, skb->data, skb->len) != skb->len) {
		printk("%s: Error writing packet data\n", dev->name);
		return 1;
	}

	dev_kfree_skb(skb);

	dev->trans_start = jiffies;

	return 0;
}

static int ux_close(struct net_device *dev)
{
	free_irq(dev->irq, dev);
	netif_stop_queue(dev);
	netif_carrier_off(dev);

	return 0;
}

static int __init ux_init(void)
{
	struct net_device *dev;
	struct ux_private *priv;
	struct ifreq ifr;
	struct l4_vhw_descriptor *vhw;
	struct l4_vhw_entry *vhwe;
	l4_kernel_info_t *kip;

	kip = l4sigma0_kip_map(L4_INVALID_ID);

	if (!l4sigma0_kip_kernel_is_ux())
		return 1;

	printk("UX ORe driver firing up...\n");

	if (!(vhw = l4_vhw_get(kip))
	    || !(vhwe = l4_vhw_get_entry_type(vhw, L4_TYPE_VHW_NET))) {
		printk(KERN_ERR "uxeth: VHW descriptor not found\n");
		return 1;
	}

	dev = init_etherdev(NULL, sizeof(struct ux_private));
	if (dev == NULL) {
		printk(KERN_ERR "uxeth: could not allocate device!\n");
		return 1;
	}

	dev->dev_addr[0] = 0x04;
	dev->dev_addr[1] = 0xEA;
	dev->dev_addr[2] = 0xDD;
	dev->dev_addr[3] = 0xFF;
	dev->dev_addr[4] = 0xFF;
	dev->dev_addr[5] = 0xFE;

	priv = netdev_priv(dev);

	dev->open            = ux_open;
	dev->hard_start_xmit = ux_start_xmit;
	dev->stop            = ux_close;
	dev->irq             = vhwe->irq_no;

	priv->fd       = vhwe->fd;
	priv->prov_pid = vhwe->provider_pid;

	return 0;
}

static void __exit ux_exit(void)
{
	printk("uxeth: exit function called, cleanup missing!\n");
}

module_init(ux_init);
module_exit(ux_exit);
