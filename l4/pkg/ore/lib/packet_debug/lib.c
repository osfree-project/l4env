/******************************************************************************
 * Bjoern Doebel <doebel@tudos.org>                                           *
 *                                                                            *
 * (c) 2007 Technische Universitaet Dresden                                   *
 * This file is part of DROPS, which is distributed under the terms of the    *
 * GNU General Public License 2. Please see the COPYING file for details.     *
 ******************************************************************************/

#include <l4/ore/packet_debug.h>
#include <l4/log/l4log.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <string.h>

#define C_ARP   "\033[31m"
#define C_ETH   "\033[32m"
#define C_IP    "\033[33m"
#define C_UDP   "\033[35m"
#define C_END   "\033[0m\n"

#define MAC_FMT	"%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_PARM(x)		(x)[0], (x)[1], (x)[2], (x)[3], (x)[4], (x)[5]

static void dump_data(unsigned char *packet, unsigned size)
{
	unsigned pos=0;
	while (pos < size) {
		LOG_printf("%02X ", packet[pos]);
		++pos;
		if (pos % 20 == 0)
			LOG_printf("\n");
	}
	LOG_printf("\n");
}

static void handle_tcp(unsigned char *packet)
{
	// XXX
}


static void handle_udp(unsigned char *packet)
{
	struct udphdr *udp = (struct udphdr *)packet;

	LOG_printf(C_UDP "[udp] port %hu -> port %hu, len %hu"C_END,
			   htons(udp->source), htons(udp->dest), htons(udp->len));
}


static void handle_icmp(unsigned char *packet)
{
	// XXX
}


static void handle_ip(unsigned char *packet)
{
	struct ip *ip = (struct ip *)packet;
	unsigned char *ip_data = packet + (packet[0] & 0x0F) * 4;

	char src[20];
	char dest[20];

	strncpy(src, inet_ntoa(ip->ip_src), 20);
	strncpy(dest, inet_ntoa(ip->ip_dst), 20);

	LOG_printf(C_IP"[ip%1x] %s -> %s, IHL %x"C_END, ip->ip_v, src, dest,
	           (packet[0] & 0x0F));
	LOG_printf(C_IP"[ip%1x] length %hu, id %X, protocol %X"C_END, ip->ip_v,
	           htons(ip->ip_len), htons(ip->ip_id), ip->ip_p);

	switch(ip->ip_p) {
		case IPPROTO_TCP:
			handle_tcp(ip_data);
			break;
		case IPPROTO_ICMP:
			handle_icmp(ip_data);
			break;
		case IPPROTO_UDP:
			handle_udp(ip_data);
			break;
		default:
			LOG_printf("FIXME: unknown transport layer protocol\n");
			break;
	}
}


static void handle_arp(unsigned char *packet)
{
	struct arphdr *arp = (struct arphdr *)packet;
	char buffer[20];
	char buffer2[20];
	unsigned char *src_mac   = packet + sizeof(struct arphdr);
	unsigned char *src_ip    = src_mac + ETH_ALEN;
	unsigned char *dest_mac  = src_ip + 4;
	unsigned char *dest_ip   = dest_mac + ETH_ALEN;

	switch(htons(arp->ar_op)) {
		case ARPOP_REQUEST:
			strncpy(buffer, "ARP request", 20);
			break;
		case ARPOP_REPLY:
			strncpy(buffer, "ARP reply", 20);
			break;
		default:
			strncpy(buffer, "don't know", 20);
			break;
	}

	LOG_printf(C_ARP"[arp] hw type %x, prot type %x, OPCODE %X (%s)"C_END,
	           htons(arp->ar_hrd), htons(arp->ar_pro), htons(arp->ar_op),
	           buffer);
	
	/* src IP */
	strncpy(buffer, inet_ntoa(*((struct in_addr*)src_ip)), 20);
	/* dest IP */
	strncpy(buffer2, inet_ntoa(*((struct in_addr*)dest_ip)), 20);

	switch(htons(arp->ar_op)) {
		case ARPOP_REQUEST:
			LOG_printf(C_ARP"[arp] Who has %s,"C_END C_ARP"\ttell "MAC_FMT" (%s)"C_END,
					   buffer2, MAC_PARM(src_mac), buffer);
			break;
		case ARPOP_REPLY:
			LOG_printf(C_ARP"[arp] %s: %s is at "MAC_FMT C_END, buffer,
					   buffer2, MAC_PARM(dest_mac));

			break;
		default:
			break;
	}
}


static void handle_eth(unsigned char *packet)
{
	struct ether_header *e = (struct ether_header *)packet;
	LOG_printf(C_ETH"[eth] "MAC_FMT" -> "MAC_FMT", proto %hx"C_END,
			   MAC_PARM(e->ether_shost), MAC_PARM(e->ether_dhost),
	           htons(e->ether_type)
	);

	switch(htons(e->ether_type))
	{
		case ETHERTYPE_IP:
			handle_ip(packet + ETH_HLEN);
			break;
		case ETHERTYPE_ARP:
			handle_arp(packet + ETH_HLEN);
			break;
		default:
			LOG("FIXME: unknown/unsupported network layer protocol\n");
			break;
	}
}


void packet_debug(unsigned char *packet)
{
	handle_eth(packet);
}
