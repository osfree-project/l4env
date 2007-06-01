/*
 * ore.c -- This file implements the ORe driver for etherboot
 *
 */

#include <stdio.h>
#include "etherboot.h"
#include "nic.h"
#include "pci.h"
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/ore/ore.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/ktrace.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>

/* set CONFIG_LOG_TRACE to 1 to trace the calls to the functions */
#define CONFIG_LOG_TRACE 0

/* set CONFIG_LOG_WARN to 1 to show error-messages */
#define CONFIG_LOG_WARN	 1

/* set CONFIG_LOG_MESSAGE to 1 to show verbose messages */
#define CONFIG_LOG_MESSAGE 0

typedef struct ore_client_state{
	unsigned char   mac[6];
	int             handle;
    l4_timeout_t    recv_to;
} ore_client_state;

ore_client_state my_state;

// extract the client state from a nic
static inline ore_client_state *get_ore_state(struct nic*nic){
    return (ore_client_state *)nic->priv_data;
}

/***********************************************************************/
/*                    Externally visible functions                     */
/***********************************************************************/

/* function: ore_transmit
 * This transmits a packet.
 *
 * Arguments: char d[6]:          destination ethernet address.
 *            unsigned short t:   ethernet protocol type.
 *            unsigned short s:   size of the data-part of the packet.
 *            char *p:            the data for the packet.
 * returns:   void.
 */
static void ore_transmit(struct nic *nic, const char *d,
                         unsigned int t, unsigned int s, const char *p)
{
  static struct {
    struct {
      unsigned char dst_addr[ETH_ALEN];
      unsigned char src_addr[ETH_ALEN];
      unsigned short type;
    } hdr;
    unsigned char data[ETH_FRAME_LEN-ETH_HLEN];
  } packet;

  int err;
  int handle = my_state.handle;

  LOGd_Enter(CONFIG_LOG_TRACE);

  /* If we sent a packet previously, wait up to 10ms until it is gone */

  memcpy(&packet.hdr.dst_addr, d, ETH_ALEN);
  memcpy(&packet.hdr.src_addr, my_state.mac, 6);
  packet.hdr.type = htons (t);
  memcpy (&packet.data, p, s);

  err = l4ore_send(handle, (char *)&packet, s + ETH_HLEN);

  if (err)
    LOGd(CONFIG_LOG_WARN, "Could not send packet: %d (%s)",
	                  err, l4env_strerror(-err));
}


/* function: ore_poll
 * This recieves a packet from the network.
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
static int ore_poll(struct nic *nic)
{
  int handle = my_state.handle;
  int ret, size = ETH_FRAME_LEN;

  LOGd_Enter(CONFIG_LOG_TRACE);

  ret = l4ore_recv_blocking(handle, (char **)&nic->packet, &size, my_state.recv_to);

  if (ret == 0)
    {
      nic->packetlen = size;

      LOGd(CONFIG_LOG_TRACE, "packet size: %d", size);
      LOGd(CONFIG_LOG_MESSAGE, "Packet header: "
		   "%02x %02x %02x %02x %02x %02x ...",
           nic->packet[0], nic->packet[1], nic->packet[2],
           nic->packet[3], nic->packet[4], nic->packet[5]);

      return size;
    }
  if (ret < 0)
    {
      LOGd(CONFIG_LOG_MESSAGE, "Could not receive packet: %d (%s)",
                               ret, l4env_strerror(-ret));
      return 0;
    }
  if (ret > 0)
    {
      LOGd(CONFIG_LOG_WARN, "rx size too small. need %d bytes in rx_buffer",
                            ret);
      return 0;
    }

  return 0;
}

static void ore_disable(struct dev* dev)
{
    LOGd_Enter(CONFIG_LOG_TRACE);
    l4ore_close(my_state.handle);
}

/* exported function: ore_probe / eth_probe
 * initializes our stub
 */

char tftp_orename[16] = "ORe";

int ore_probe(struct dev *dev, const char *type_name);
int ore_probe(struct dev *dev, const char *type_name)
{

  struct nic *nic        = (struct nic*)dev;
  int        err         = 0;
  static int initialized = PROBE_FAILED;
  l4ore_config ore_conf  = L4ORE_DEFAULT_CONFIG;
  int exp, mant;

#if 1
  ore_conf.ro_keep_device_mac = 1;
#endif

  LOGd_Enter(CONFIG_LOG_TRACE, "%s initialized",
              initialized == PROBE_WORKED ? "already" : "not yet");

  if (initialized == PROBE_WORKED)
    return initialized;

  // accept broadcasts as we want to answer ARP requests
  ore_conf.rw_broadcast = 1;
  strncpy(ore_conf.ro_orename, tftp_orename, sizeof(tftp_orename));
  tftp_orename[sizeof(tftp_orename)-1] = 0;
  LOGd(CONFIG_LOG_TRACE, "Connecting to ORe instance %s", tftp_orename);
  
  err = l4ore_open("eth0", nic->node_addr, &ore_conf);
  if (err < 0)
    {
      LOGdl(CONFIG_LOG_WARN, "ore_open_string(): %s", l4env_strerror(-err));
      return PROBE_FAILED;
    }

  LOGdl(CONFIG_LOG_TRACE, "opened ORe channel: %d", err);
  l4util_micros2l4to(500000, &mant, &exp);
  my_state.handle = err;
  memcpy(my_state.mac, nic->node_addr, 6);
  my_state.recv_to = l4_timeout(0,0,mant,exp);

  initialized    = PROBE_WORKED;
  nic->priv_data = (void*)&my_state;

  nic->poll      = ore_poll;
  nic->transmit  = ore_transmit;
  dev->disable   = ore_disable;

  return initialized;
}

/*
 * Local variables:
 *   compile-command: "make -C .."
 * End:
 */
