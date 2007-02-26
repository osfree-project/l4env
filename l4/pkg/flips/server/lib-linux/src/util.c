/* A bunch of utilities (mostly for debugging) */

#include <linux/socket.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/if_ether.h>

#include <net/ip.h>
#include <net/protocol.h>
#include <net/route.h>

#include "local.h"

/** ANALYZE SKBUFF AND PRINT SUMMARY */
void liblinux_skb_stats(struct sk_buff *skb, int rx)
{
	unsigned short proto;
	unsigned char *data = skb->data;
	static char stats[256];

	struct ethhdr *mac;
	struct iphdr *ip;
	struct tcphdr *tcp;

	Assert(skb);

	/* prepare packet */
	if (rx) {                        /* MAC layer (802.3) */
		mac = skb->mac.ethernet;
	} else {
		mac = (struct ethhdr *)data;
		data += ETH_HLEN;
	}
	ip = (struct iphdr *)data;       /* network layer if Ethernet II */
	data += sizeof(struct iphdr);
	tcp = (struct tcphdr *)data;     /* transport layer if IP */

	switch ((proto = ntohs(mac->h_proto))) {
	case ETH_P_IP:
		switch (ip->protocol) {
		case IPPROTO_TCP:
			snprintf(stats, 256, "TCP:%s%s%s%s%s",
			         tcp_flag_word(tcp) & TCP_FLAG_SYN ? " SYN" : "",
			         tcp_flag_word(tcp) & TCP_FLAG_FIN ? " FIN" : "",
			         tcp_flag_word(tcp) & TCP_FLAG_RST ? " RST" : "",
			         tcp_flag_word(tcp) & TCP_FLAG_PSH ? " PSH" : "",
			         tcp_flag_word(tcp) & TCP_FLAG_ACK ? " ACK" : "");
			break;
		case IPPROTO_UDP:
			snprintf(stats, 256, "UDP");
			break;
		case IPPROTO_ICMP:
			snprintf(stats, 256, "ICMP");
			break;
		default:
			snprintf(stats, 256, "unknown IP protocol %d",
			            ip->protocol);
		}
		break;
	case ETH_P_ARP:
		snprintf(stats, 256, "ARP");
		break;
	default:
		snprintf(stats, 256, " +++ unknown protocol 0x%04x +++ ", proto);
	}

	printk("%s: %s %d bytes (%s):\n", skb->dev->name,
	       rx ? "rx" : "tx", skb->len, stats);

	if (0) {
		int j = 0;
		data = (unsigned char *)mac;

		printk("  ");
		for (; j < 8; j++)
			printk(" %02x", data[j]);
		printk(" ");
		for (; j < 16; j++)
			printk(" %02x", data[j]);
		printk("\n  ");
		for (; j < 24; j++)
			printk(" %02x", data[j]);
		printk(" ");
		for (; j < 32; j++)
			printk(" %02x", data[j]);
		printk("\n");
	}
}

/** PRINT STRING FOR netif_rx() RETURN VALUE */
void liblinux_check_netif_rx(struct net_device *dev, int value)
{
	switch (value) {
	case NET_RX_SUCCESS:
		break;
	case NET_RX_CN_LOW:
		printk("%s: low congestion\n", dev->name);
		break;
	case NET_RX_CN_MOD:
		printk("%s: moderate congestion\n", dev->name);
		break;
	case NET_RX_CN_HIGH:
		printk("%s: high congestion\n", dev->name);
		break;
	case NET_RX_DROP:
		printk("%s: packet was dropped\n", dev->name);
		break;
	default:
		printk("%s: netifrx() returns %d\n", dev->name, value);
	}
}
