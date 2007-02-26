#include <stdlib.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/ore/ore.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PROT_ICMP         1
#define ICMP_REPLY        0
#define ICMP_REQ          8
#define ETH_ALEN          6

#define REPLY_PACKET_LEN  80
#define RECV_PACKET_LEN   500

#define	ARPING_VERBOSE    0

#define RECV_BROADCAST    0

char LOG_tag[9] = "arping";
l4_ssize_t l4libc_heapsize = 32 * 1024;
static unsigned char broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct ethernet_hdr
{
  unsigned char dest[6];
  unsigned char src[6];
  unsigned char type[2];
};

struct ip_hdr
{
  char version_length;
  char type;
  short length;
  short id;
  short flags_offset;
  char ttl;
  char protocol;
  short checksum;
  l4_int32_t src_ip;
  l4_int32_t dest_ip;
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

int parse_icmp_packet(char *, struct ethernet_hdr *,
                      struct ip_hdr *, struct icmp_hdr *);
int parse_icmp_packet(char *rx_buf, struct ethernet_hdr *eth_hdr,
                      struct ip_hdr *ip_hdr, struct icmp_hdr *icmp_hdr)
{
  struct ethernet_hdr *eth = NULL;
  struct ip_hdr *ip        = NULL;
  struct icmp_hdr *icmp    = NULL;
  int ver, len;

  eth = (struct ethernet_hdr *)rx_buf;
  LOGd(ARPING_VERBOSE, "dest mac = %02x:%02x:%02x:%02x:%02x:%02x",
                       eth->dest[0], eth->dest[1], eth->dest[2],
                       eth->dest[3], eth->dest[4], eth->dest[5]);

  LOGd(ARPING_VERBOSE, "src mac = %02x:%02x:%02x:%02x:%02x:%02x",
                       eth->src[0], eth->src[1], eth->src[2],
                       eth->src[3], eth->src[4], eth->src[5]);
  LOGd(ARPING_VERBOSE, "type field = %02x%02x", eth->type[0], eth->type[1]);

  ip = (struct ip_hdr *)(rx_buf + sizeof(struct ethernet_hdr));

  if (memcmp(eth->dest, broadcast_mac, 6) == 0)
    {
      LOG("broadcast packet - dropping.");
      return -1;
    }

  if (ip->protocol != PROT_ICMP)
    {
      LOG("This does not look like an ICMP packet. Dropping...");
      return -1;
    }

  LOGd(ARPING_VERBOSE, "ICMP packet:");

  ver = ip->version_length >> 4;
  len = ip->version_length & 0x0F;
  LOGd(ARPING_VERBOSE, "IP version = %d, length = %d", ver, len);

  //ip->src_ip  = ntohl(ip->src_ip);
  //ip->dest_ip = ntohl(ip->dest_ip);
  LOGd(ARPING_VERBOSE, "dest IP = %s",
                       inet_ntoa(*(struct in_addr *)&ip->dest_ip));
  LOGd(ARPING_VERBOSE, "src IP  = %s",
                       inet_ntoa(*(struct in_addr *)&ip->src_ip));

  icmp = (struct icmp_hdr *)(rx_buf + sizeof(struct ethernet_hdr)
         + sizeof(struct ip_hdr));

  if (icmp->type != ICMP_REQ)
    {
      LOG("This is no ICMP request.");
      return -1;
    }

  LOGd(ARPING_VERBOSE, "Hey this is an ICMP request just for me. :)");
  LOGd(ARPING_VERBOSE, "ICMP type : %d", icmp->type);
  LOGd(ARPING_VERBOSE, "ICMP code : %d", icmp->code);
  LOGd(ARPING_VERBOSE, "ICMP seq  : %d", ntohs(icmp->seq_num));

  // copy values
  *eth_hdr  = *eth;
  *ip_hdr   = *ip;
  *icmp_hdr = *icmp;

  return 0;
}

void create_icmp_reply(char *buf, struct ethernet_hdr *eth, struct ip_hdr *ip,
                       struct icmp_hdr *icmp);
void create_icmp_reply(char *buf, struct ethernet_hdr *eth, struct ip_hdr *ip,
                       struct icmp_hdr *icmp)
{
  struct ethernet_hdr *e;
  struct ip_hdr       *iphdr;
  struct icmp_hdr     *i;

  // ETHERNET HEADER
  e = (struct ethernet_hdr *)buf;
  // switch src and dest address
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

int main(int argc, char **argv)
{
  unsigned char mac[6];
  struct ethernet_hdr eth;
  struct ip_hdr	      ip;
  struct icmp_hdr     icmp;
  int handle, size;
  char *rx_buf = (char *)malloc(RECV_PACKET_LEN);
  char *tx_buf = (char *)malloc(REPLY_PACKET_LEN);
  l4ore_config ore_conf = L4ORE_DEFAULT_CONFIG;

  LOG("Hello from the ORe arping client");
  //LOG("My name is " l4util_idfmt, l4util_idstr(l4_myself()));

  // clean buffers
  memset(rx_buf, 0, RECV_PACKET_LEN);
  memset(tx_buf, 0, REPLY_PACKET_LEN);

#if RECV_BROADCAST
  ore_conf.rw_broadcast = 1;
#endif
  
  handle = l4ore_open("eth0", mac, NULL, NULL, &ore_conf);
  LOG("opened eth0: %d for %02X:%02X:%02X:%02X:%02X:%02X", handle,
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  while (1)
    {
      size = RECV_PACKET_LEN;
      LOGd(ARPING_VERBOSE, "receiving...");
      l4ore_recv_blocking(handle, &rx_buf, &size);
      LOGd(ARPING_VERBOSE, "received packet of length %d", size);

      if (parse_icmp_packet(rx_buf, &eth, &ip, &icmp) == 0)
        {
          LOGd(ARPING_VERBOSE, "correctly parsed ICMP packet.");
          create_icmp_reply(tx_buf, &eth, &ip, &icmp);
          LOG("sending ICMP reply");
          l4ore_send(handle, tx_buf, REPLY_PACKET_LEN);
        }
    }

  LOG("Finished. Going to sleep.");
  l4_sleep_forever();
  return 0;
}
