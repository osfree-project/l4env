/*
 * oshkosh.c -- This file implements the OshKosh driver for etherboot
 *
 */

#include <stdio.h>
#include "etherboot.h"
#include "nic.h"
#include "pci.h"
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/sys/kdebug.h>
#include <l4/oshkosh/beapi.h>
#include <l4/sys/ktrace.h>

/* set CONFIG_LOG_TRACE to 1 to trace the calls to the functions */
#define CONFIG_LOG_TRACE 0

/* set CONFIG_LOG_WARN to 1 to show error-messages */
#define CONFIG_LOG_WARN	 1

/* set CONFIG_LOG_MESSAGE to 1 to show verbose messages */
#define CONFIG_LOG_MESSAGE 0

static inline oshk_client_conn_t* oshk_conn(struct nic*nic){
    return (oshk_client_conn_t*)nic->priv_data;
}

/***********************************************************************/
/*                    Externally visible functions                     */
/***********************************************************************/

/* function: oshkosh_transmit
 * This transmits a packet.
 *
 * Arguments: char d[6]:          destination ethernet address.
 *            unsigned short t:   ethernet protocol type.
 *            unsigned short s:   size of the data-part of the packet.
 *            char *p:            the data for the packet.
 * returns:   void.
 */
static void oshkosh_transmit(struct nic *nic, const char *d,
			     unsigned int t, unsigned int s, const char *p){
    static struct{
	struct {
	    unsigned char dst_addr[ETH_ALEN];
	    unsigned char src_addr[ETH_ALEN];
	    unsigned short type;
	} hdr;
	unsigned char data[ETH_FRAME_LEN-ETH_HLEN];
    } packet;
    static oshk_client_tx_t desc;
    int err;

    LOGd_Enter(CONFIG_LOG_TRACE);

    /* If we sent a packet previously, wait up to 10ms until it is gone */
    if(desc.addr){
	while(oshk_client_check_tx(oshk_conn(nic), &desc)){
	    if((err=oshk_client_be_waitv(oshk_conn(nic), 1, 1, 0, 0,
				         10000, 0))!=0){
		LOGd(CONFIG_LOG_WARN,
		     "Timeout, old packet not sent: %s", l4env_strerror(-err));
		return;
	    }
	}
    }

    memcpy(&packet.hdr.dst_addr, d, ETH_ALEN);
    memcpy(&packet.hdr.src_addr, nic->node_addr, 6);
    packet.hdr.type = htons (t);
    memcpy (&packet.data, p, s);
    desc.addr = &packet;
    desc.len = s+ETH_HLEN;
    
    if((err=oshk_client_be_put_tx_desc(oshk_conn(nic), &desc))!=0){
	LOGd(CONFIG_LOG_WARN,
	     "Cannot send packet: %s", l4env_strerror(-err));
	return;
    }
}


/* function: oshkosh_poll
 * This recieves a packet from the network. 
 * When this function is called to drain the rx-queue, it does not wait.
 * Otherwise, it waits up to 1s for the arrival of new frames.
 *
 * Arguments: none
 *
 * returns:   1 if a packet was recieved.
 *            0 if no pacet was recieved.
 * side effects:
 *            returns the packet in the array nic->packet.
 *            returns the length of the packet in nic->packetlen.
 */
extern int rx_drain_flag;
static int oshkosh_poll(struct nic *nic){
    oshk_client_be_rx_t desc;
    int ret;

    LOGd_Enter(CONFIG_LOG_TRACE);

    desc.addr = nic->packet;
    if((ret = oshk_client_be_waitv(oshk_conn(nic),
				   !rx_drain_flag,
				   0, 1, &desc,
				   1000000, 0))<0){
	if(!rx_drain_flag){
	    LOGdl(CONFIG_LOG_WARN || CONFIG_LOG_TRACE,
		  "oshk_client_waitv: %s", l4env_strerror(-ret));
	}
	return 0;
    }


    if(ret==0){
	LOGd(CONFIG_LOG_MESSAGE, "waitv(): no packet, in time.");
	return 0;
    }
    LOGd(CONFIG_LOG_MESSAGE, "Got packet %p+%d", desc.addr, desc.len);

    nic->packetlen =  desc.len;
    return 1;
}

static void oshkosh_disable(struct dev* dev){
    LOGd_Enter(CONFIG_LOG_TRACE);
    
    oshk_client_be_close(oshk_conn((struct nic*)dev), 1);
}

/* exported function: oshkosh_probe / eth_probe
 * initializes our stub
 */

int oshkosh_probe(struct dev *dev, const char*type_name){
    struct nic*nic = (struct nic*)dev;
    oshk_client_conn_t *conn;
    int err;
    static int initialized=PROBE_FAILED;

    LOGd_Enter(CONFIG_LOG_TRACE, "%s initialized",
	       initialized==PROBE_WORKED?"already":"not yet");
    if(initialized==PROBE_WORKED) return initialized;

    dsi_init();    
    if((err = oshk_client_be_open("eth0", nic->node_addr, 0, &conn, 1))!=0){
	LOGdl(CONFIG_LOG_WARN,
	      "oshk_client_open: %s", l4env_strerror(-err));
	return PROBE_FAILED;
    }
    initialized=PROBE_WORKED;
    nic->priv_data = (void*)conn;

    nic->poll = oshkosh_poll;
    nic->transmit = oshkosh_transmit;
    dev->disable = oshkosh_disable;

    return initialized;
}


/*
 * Local variables:
 *   compile-command: "make -C .."
 * End:
 */
