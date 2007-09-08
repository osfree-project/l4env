/****************************************************************
 * (c) 2007 Technische Universitaet Dresden                     *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/sys/ipc.h>

#include <linux/netdevice.h>
#include <linux/if_ether.h>

#define PROT_ICMP         1
#define ICMP_REPLY        0
#define ICMP_REQ          8
#define ETH_ALEN          6

/* configuration */
static int arping_verbose = 1;  // verbose

char LOG_tag[9] = "arping";
l4_ssize_t l4libc_heapsize = 32 * 1024;

static unsigned char broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static int exit_somewhen = 0;


struct ethernet_hdr
{
	unsigned char dest[6];
	unsigned char src[6];
	unsigned char type[2];
};


struct ip_hdr
{
	char          version_length;
	char          type;
	l4_int16_t    length;
	l4_int16_t    id;
	l4_int16_t    flags_offset;
	char          ttl;
	char          protocol;
	l4_int16_t    checksum;
	l4_int32_t    src_ip;
	l4_int32_t    dest_ip;
};


struct icmp_hdr
{
	char type;
	char code;
	l4_uint16_t checksum;
	l4_uint16_t id;
	l4_uint16_t seq_num;
};


static int handle_icmp_packet(struct sk_buff *skb);
static int handle_icmp_packet(struct sk_buff *skb)
{
	char *data = skb->data;
	struct ethernet_hdr *eth = NULL;
	struct ethernet_hdr *e   = NULL;
	struct ip_hdr *ip        = NULL;
	struct ip_hdr *iphdr     = NULL;
	struct icmp_hdr *icmp    = NULL;
	struct icmp_hdr *icmp2   = NULL;
	int ver, len;
	struct sk_buff *snd_skb  = NULL;

	eth = (struct ethernet_hdr *)data;
	LOGd(arping_verbose, "dest mac = %02x:%02x:%02x:%02x:%02x:%02x",
	     eth->dest[0], eth->dest[1], eth->dest[2],
	     eth->dest[3], eth->dest[4], eth->dest[5]);
	LOGd(arping_verbose, "src mac = %02x:%02x:%02x:%02x:%02x:%02x",
	     eth->src[0], eth->src[1], eth->src[2],
	     eth->src[3], eth->src[4], eth->src[5]);
	LOGd(arping_verbose, "type field = %02x%02x", eth->type[0], eth->type[1]);
	if (eth->type[0] != 0x08 || eth->type[1] != 0x00) {
		LOG("unknown ethernet packet type!");
		return -1;
	}

	ip = (struct ip_hdr *)(data + sizeof(struct ethernet_hdr));
		LOGd(arping_verbose, "protocol = %02x (== %02x?)", ip->protocol, PROT_ICMP);
		if (ip->protocol != PROT_ICMP)
	{
		LOG("Unknown packet type.");
		return -1;
	}

	LOGd(arping_verbose, "ICMP packet!");
	ver = ip->version_length >> 4;
	len = ip->version_length & 0x0F;
	LOGd(arping_verbose, "IP version = %d, length = %d", ver, len);

	LOG("src IP: "NIPQUAD_FMT, NIPQUAD(ip->src_ip));
	LOG("dest IP: "NIPQUAD_FMT, NIPQUAD(ip->dest_ip));

	icmp = (struct icmp_hdr *)(data + sizeof(struct ethernet_hdr)
	        + sizeof(struct ip_hdr));

	if (icmp->type != ICMP_REQ)
	{
		LOG("This is no ICMP request.");
		return -1;
	}
	LOGd(arping_verbose, "Hey this is an ICMP request just for me. :)");
	LOGd(arping_verbose, "ICMP type : %d", icmp->type);
	LOGd(arping_verbose, "ICMP code : %d", icmp->code);
	LOGd(arping_verbose, "ICMP seq  : %d", ntohs(icmp->seq_num));

	snd_skb = alloc_skb(skb->len + skb->dev->hard_header_len, GFP_KERNEL);
	memcpy(snd_skb->data, skb->data, skb->len);
	
	e = (struct ethernet_hdr *)snd_skb->data;
	memcpy(e->src, eth->dest, ETH_ALEN);
	memcpy(e->dest, eth->src, ETH_ALEN);
	LOGd(arping_verbose, "dest mac = %02x:%02x:%02x:%02x:%02x:%02x",
	     e->dest[0], e->dest[1], e->dest[2],
	     e->dest[3], e->dest[4], e->dest[5]);
	LOGd(arping_verbose, "src mac = %02x:%02x:%02x:%02x:%02x:%02x",
	     e->src[0], e->src[1], e->src[2],
	     e->src[3], e->src[4], e->src[5]);
	e->type[0] = 0x08;
	e->type[1] = 0x00;

	iphdr  = (struct ip_hdr *)(snd_skb->data + sizeof(struct ethernet_hdr));
	*iphdr = *ip;
	// also switch src and dest
	iphdr->src_ip  = ip->dest_ip;
	iphdr->dest_ip = ip->src_ip;
	LOG("src IP: "NIPQUAD_FMT, NIPQUAD(iphdr->src_ip));
	LOG("dest IP: "NIPQUAD_FMT, NIPQUAD(iphdr->dest_ip));

	icmp2 = (struct icmp_hdr *)(snd_skb->data + sizeof(struct ethernet_hdr)
	                            + sizeof(struct ip_hdr));
	*icmp2     = *icmp;
	icmp2->type = ICMP_REPLY;

	snd_skb->dev = skb->dev;
	snd_skb->len = skb->len;

	LOG("sending reply");
	skb->dev->hard_start_xmit(snd_skb, skb->dev);
	LOG("done");

	return 0;
}


int arping(void)
{
	LOG("there we go...");
	while(1)
	{
		l4_threadid_t src;
		l4_msgdope_t res;
		l4_umword_t dw0, dw1;
		struct sk_buff *skb;
		int err;

		/* await notification */
		err = l4_ipc_wait(&src, L4_IPC_SHORT_MSG,
		                  &dw0, &dw1, L4_IPC_NEVER, &res);
		LOG("ipc_wait: %d", err);

		skb = (struct sk_buff *)dw0;
		skb_get(skb);

		err = l4_ipc_send(src, L4_IPC_SHORT_MSG,
		                  0, 0, L4_IPC_NEVER, &res);
		LOG("ipc_send: %d", err);

		/* parse packet */
		err = handle_icmp_packet(skb);
		LOG("handle_icmp_packet: %d", err);
	}
}

