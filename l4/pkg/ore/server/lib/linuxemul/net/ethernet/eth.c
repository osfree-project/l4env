/*!
 * \file   ore/server/lib/linuxemul/net/ethernet/eth.c
 * \brief  Ethernet-type device handling emulation
 *
 * \date   06/28/2005
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *         Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 *
 * We provide some dummy functions as they are used by drivers to fill
 * in device-specific level 2-3 adapter functions. However, as OshKosh
 * acts as a raw device, we should not need this.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <linux/config.h>
#include <linux/init.h>
#include <net/dst.h>
#include <net/arp.h>
#include <net/sock.h>
#include <net/ipv6.h>
#include <net/ip.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/checksum.h>
#include <l4/log/l4log.h>

extern int __init netdev_boot_setup(char *str);

__setup("ether=", netdev_boot_setup);

/*
 *	 Create the Ethernet MAC header for an arbitrary protocol layer 
 *
 *	saddr=NULL	means use device source address
 *	daddr=NULL	means leave destination address (eg unresolved arp)
 */

int eth_header(struct sk_buff *skb, struct net_device *dev,
	       unsigned short type, void *daddr, void *saddr, unsigned len){

    LOG_Error("Unimplemented");
    return 0;
}


/*
 *	Rebuild the Ethernet MAC header. This is called after an ARP
 *	(or in future other address resolution) has completed on this
 *	sk_buff. We now let ARP fill in the other fields.
 *
 *	This routine CANNOT use cached dst->neigh!
 *	Really, it is used only when dst->neigh is wrong.
 */

int eth_rebuild_header(struct sk_buff *skb){
    LOG_Error("Unimplemented");
    return 0;
}


/*
 *	Determine the packet's protocol ID. The rule here is that we 
 *	assume 802.3 if the type field is short enough to be a length.
 *	This is normal practice and works for any 'now in use' protocol.
 */
 
unsigned short eth_type_trans(struct sk_buff *skb, struct net_device *dev){
        struct ethhdr *eth; 
        unsigned char *rawp;
        
        skb->mac.raw=skb->data;
        skb_pull(skb,dev->hard_header_len);
        eth= skb->mac.ethernet;

        if(*eth->h_dest&1)
        {
                if(memcmp(eth->h_dest,dev->broadcast, ETH_ALEN)==0)
                        skb->pkt_type=PACKET_BROADCAST;
                else
                        skb->pkt_type=PACKET_MULTICAST;
        } 
          
        /*
         *      This ALLMULTI check should be redundant by 1.4
         *      so don't forget to remove it.
         *
         *      Seems, you forgot to remove it. All silly devices
         *      seems to set IFF_PROMISC.
         */
         
        else if(1 /*dev->flags&IFF_PROMISC*/)
        {
                if(memcmp(eth->h_dest,dev->dev_addr, ETH_ALEN))
                        skb->pkt_type=PACKET_OTHERHOST;
        }

        if (ntohs(eth->h_proto) >= 1536)
                return eth->h_proto;
                
        rawp = skb->data;
          
        /*
         *      This is a magic hack to spot IPX packets. Older Novell breaks  
         *      the protocol design and runs IPX over 802.3 without an 802.2 LL
         *      layer. We look for FFFF which isn't a used 802.2 SSAP/DSAP. Thi
         *      won't work for fault tolerant netware but does for the rest.
         */
        if (*(unsigned short *)rawp == 0xFFFF)
                return htons(ETH_P_802_3);
          
        /*
         *      Real 802.2 LLC
         */
        return htons(ETH_P_802_2);
}

int eth_header_parse(struct sk_buff *skb, unsigned char *haddr){
    LOG_Error("Unimplemented");
    return 0;
}

int eth_header_cache(struct neighbour *neigh, struct hh_cache *hh){
    LOG_Error("Unimplemented");
    return 0;
}

/*
 * Called by Address Resolution module to notify changes in address.
 */

void eth_header_cache_update(struct hh_cache *hh,
			     struct net_device *dev, unsigned char * haddr){
    LOG_Error("Unimplemented");
}

/*
 * Local variables:
 *  compile-command: "make -C ../.."
 * End:
 */
