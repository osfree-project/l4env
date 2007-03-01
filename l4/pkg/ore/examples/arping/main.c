/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/ore/ore.h>
#include <l4/util/getopt.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/l4_macros.h>
#include <l4/sys/ipc.h>
#include <l4/dm_generic/types.h>
#include <l4/dm_mem/dm_mem.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PROT_ICMP         1
#define ICMP_REPLY        0
#define ICMP_REQ          8
#define ETH_ALEN          6

#define REPLY_PACKET_LEN  80
#define RECV_PACKET_LEN   500
#define DATASPACE_SIZE    (1024 * 1024)

/* configuration */
static int arping_verbose = 0;  // verbose
static int recv_broadcast = 0;  // recv broadcast packets
static char *ore_name     = NULL;
static int use_phys       = 0;
//static int send_dsi       = 0;  // send through dsi
//static int recv_dsi       = 0;  // receive through dsi

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

void testlog(char *s);
void testlog(char *s)
{
    LOG("\033[32m%s\033[0m",s);
}

static int parse_icmp_packet(char *rx_buf, struct ethernet_hdr *eth_hdr,
                             struct ip_hdr *ip_hdr, struct icmp_hdr *icmp_hdr)
{
    struct ethernet_hdr *eth = NULL;
    struct ip_hdr *ip        = NULL;
    struct icmp_hdr *icmp    = NULL;
    int ver, len;

    eth = (struct ethernet_hdr *)rx_buf;
    LOGd(arping_verbose, "dest mac = %02x:%02x:%02x:%02x:%02x:%02x",
                       eth->dest[0], eth->dest[1], eth->dest[2],
                       eth->dest[3], eth->dest[4], eth->dest[5]);

    LOGd(arping_verbose, "src mac = %02x:%02x:%02x:%02x:%02x:%02x",
                       eth->src[0], eth->src[1], eth->src[2],
                       eth->src[3], eth->src[4], eth->src[5]);
    LOGd(arping_verbose, "type field = %02x%02x", eth->type[0], eth->type[1]);

    ip = (struct ip_hdr *)(rx_buf + sizeof(struct ethernet_hdr));
    LOGd(arping_verbose, "protocol = %02x (== %02x)", ip->protocol, PROT_ICMP);

    if (memcmp(eth->dest, broadcast_mac, 6) == 0)
    {
        LOG("broadcast packet - dropping.");
        return -1;
    }

    if (ip->protocol != PROT_ICMP)
    {
        LOG("Unknown packet type.");
        return -1;
    }

    LOGd(arping_verbose, "ICMP packet:");

    ver = ip->version_length >> 4;
    len = ip->version_length & 0x0F;
    LOGd(arping_verbose, "IP version = %d, length = %d", ver, len);

    //ip->src_ip  = ntohl(ip->src_ip);
    //ip->dest_ip = ntohl(ip->dest_ip);
    LOGd(arping_verbose, "dest IP = %s",
                       inet_ntoa(*(struct in_addr *)&ip->dest_ip));
    LOGd(arping_verbose, "src IP  = %s",
                       inet_ntoa(*(struct in_addr *)&ip->src_ip));

    icmp = (struct icmp_hdr *)(rx_buf + sizeof(struct ethernet_hdr)
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

    // copy values
    *eth_hdr  = *eth;
    *ip_hdr   = *ip;
    *icmp_hdr = *icmp;

    return 0;
}

static void create_icmp_reply(void *buf, struct ethernet_hdr *eth,
                              struct ip_hdr *ip, struct icmp_hdr *icmp)
{
    struct ethernet_hdr *e;
    struct ip_hdr       *iphdr;
    struct icmp_hdr     *i;

    // ETHERNET HEADER
    e = (struct ethernet_hdr *)buf;
    // swap src and dest address
    memcpy(e->src, eth->dest, ETH_ALEN);
    memcpy(e->dest, eth->src, ETH_ALEN);
    e->type[0] = 0x08;
    e->type[1] = 0x00;

    // IP HEADER
    iphdr  = (struct ip_hdr *)(buf + sizeof(struct ethernet_hdr));
    *iphdr = *ip;
    // also switch src and dest
    iphdr->src_ip  = ip->dest_ip;
    iphdr->dest_ip = ip->src_ip;

    i = (struct icmp_hdr *)(iphdr + sizeof(struct ip_hdr));
    *i         = *icmp;
    i->type    = ICMP_REPLY;
}

int main(int argc, const char **argv)
{
    unsigned char mac[6];
    struct ethernet_hdr eth;
    struct ip_hdr       ip;
    struct icmp_hdr     icmp;
    int handle, cnt=0, ret __attribute__((unused));
    int retval[4];
    unsigned int size;
    char *rx_buf;
    void *send_start;
    void *next_addr;
//    l4dm_dataspace_t recv_ds = L4DM_INVALID_DATASPACE;  
//    l4dm_dataspace_t send_ds = L4DM_INVALID_DATASPACE;
  
    l4ore_config ore_conf = L4ORE_DEFAULT_CONFIG;

    LOG("Hello from the ORe arping shared memory client");

    if (parse_cmdline(&argc, &argv,
              'b', "broadcast", "receive broadcast packets",
              PARSE_CMD_SWITCH, 1, &recv_broadcast,
              'e', "exit", "exit after some pings",
              PARSE_CMD_SWITCH, 1, &exit_somewhen,
              'o', "orename", "name of ORe instance to connect to",
              PARSE_CMD_STRING, "ORe", &ore_name,
			  'p', "phys", "use physical MAC address if possible",
			  PARSE_CMD_SWITCH, 1, &use_phys,
              'v', "verbose", "verbose output",
              PARSE_CMD_SWITCH, 1, &arping_verbose,
//              's', "senddsi", "send packets via dsi",
//              PARSE_CMD_SWITCH, 1, &send_dsi,
//              'r', "recvdsi", "receive packets via dsi",
//              PARSE_CMD_SWITCH, 1, &recv_dsi,
              0,0))
        return 1;

    if (ore_name)
    {
        LOG("connecting to '%s'", ore_name);
        strcpy(ore_conf.ro_orename, ore_name);
    }
    else
        LOG("connecting to 'ORe'");

    if (recv_broadcast)
        ore_conf.rw_broadcast = 1;

	if (use_phys) {
		LOG("using physical device mac.");
		ore_conf.ro_keep_device_mac = 1;
	}
#if 0  
    if (recv_dsi)
    {
        LOG("receiving via shared memory");
        ret = l4dm_mem_open(L4DM_DEFAULT_DSM, DATASPACE_SIZE, 0,
              L4DM_CONTIGUOUS | L4DM_PINNED, "arping recv dataspace", &recv_ds);
        LOG("created recv dataspace: %d", ret);
        ore_conf.ro_recv_ds = recv_ds;
    }
    else
    {
#endif
        LOG("string ipc receive");
        rx_buf = malloc(RECV_PACKET_LEN);
        memset(rx_buf, 0, RECV_PACKET_LEN);
#if 0
    }

    if (send_dsi)
    {
        LOG("sending via shared memory");
        ret = l4dm_mem_open(L4DM_DEFAULT_DSM, DATASPACE_SIZE, 0,
              L4DM_CONTIGUOUS | L4DM_PINNED, "arping send dataspace", &send_ds);
        LOG("created send dataspace: %d", ret);
        ore_conf.ro_send_ds   = send_ds;
    }
    else
#endif
        LOG("sending with string ipc");


    handle = l4ore_open("eth0", mac, &ore_conf);
    LOG("opened eth0: %d for %02X:%02X:%02X:%02X:%02X:%02X", handle,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (handle < 0)
    {
        LOG_Error("could not open eth0!");
        return 1;
    }

    LOG("ORe handle = %d", handle);

	if (use_phys) {
		LOG("%s physical device MAC.",
		    ore_conf.ro_keep_device_mac == 1 ? "got" : "didn't get");
	}

#if 0
    if (send_dsi)
    {
        send_start            = l4ore_get_send_area(handle);
    }
    else
    {
#endif
        send_start            = malloc(DATASPACE_SIZE);  
#if 0
    }
#endif

    next_addr     = send_start;
    LOG("got send area: %p", next_addr);
  
    while (1)
    {
        if (exit_somewhen)
        {
            cnt++;
            if (cnt > 10)
                break;
        }
      
        size = RECV_PACKET_LEN;
        LOGd(arping_verbose, "receiving...");
        retval[0] = l4ore_recv_blocking(handle, &rx_buf, &size, L4_IPC_NEVER);
        if (retval[0])
            continue;
        LOGd(arping_verbose, "received packet of length %d at %p", size, rx_buf);

        retval[1] = parse_icmp_packet(rx_buf, &eth, &ip, &icmp);
        if (!retval[1])
        {
#if 0
            if (recv_dsi)
            {
                LOGd(arping_verbose, "returning packet as we do not need it anymore");
                l4ore_packet_to_pool(rx_buf);
            }
#endif
            LOGd(arping_verbose, "correctly parsed ICMP packet.");
            create_icmp_reply((char *)next_addr, &eth, &ip, &icmp);
            LOGd(arping_verbose, "sending ICMP reply %p", next_addr);
            retval[3] = l4ore_send(handle, next_addr, REPLY_PACKET_LEN);
            next_addr += REPLY_PACKET_LEN;
            if (next_addr >= (send_start + DATASPACE_SIZE - REPLY_PACKET_LEN))
                next_addr = send_start;
        }
    }

    LOG("arping client going down.");
    return 0;
}

