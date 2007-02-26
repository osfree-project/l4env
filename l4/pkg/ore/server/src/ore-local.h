/****************************************************************
 * ORe local declarations.										*
 *																*
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>					*
 * 2005-08-10													*
 ****************************************************************/

#ifndef __ORE_LOCAL_H
#define __ORE_LOCAL_H

#include <l4/log/l4log.h>
#include <l4/ore/ore.h>
#include <l4/dm_generic/types.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/sys/types.h>
#include <dice/dice.h>

#include "config.h"
#include "linuxemul.h"
#include "auto_config.h"
#include <l4/ore/main-server.h>
#include <l4/ore/ore-server.h>
#include <l4/ore/ore_notify-server.h>
#include <l4/ore/ore_notify-client.h>

#include <linux/netdevice.h>

/* this is the kind of data we are going to store in the rx
 * and tx lists */
typedef struct rxtx_entry_t
{
    struct list_head list;     // the list we are on
    // FIXME: call this ->skb
    struct sk_buff   *buf;     // socket buffer
} rxtx_entry_t;

typedef unsigned char ore_mac[6];

/* connection state used by ORe */
typedef struct ore_connection_t{
    int               in_use;         // connection in use?
    struct net_device *dev;           // opened device
    ore_mac           mac;            // allocated MAC address
    // TODO: make rx_list and recv_ds a union
    l4dm_dataspace_t  recv_ds;        // receive dataspace
    // TODO: make tx_list and send_ds a union
    l4dm_dataspace_t  send_ds;        // send dataspace
    struct list_head  rx_list;        // list for rx packets if we don't have
    // a rx_dataspace
    struct list_head  tx_list;        // list for tx packets if we don't have
    // a tx_dataspace
    l4lock_t          channel_lock;   // channel lock
    l4_int32_t        flags;          // server-side connection flags
    l4ore_config      config;         // client-side configuration
    l4_threadid_t     waiting_client; // currently waiting client
    int               waiting_size;   // buffer size of the currently waiting client
    // points to the real rx component function
    int               (*rx_component_func)(CORBA_Object, l4ore_handle_t,
                                           char **, l4_umword_t *,
                                           l4_umword_t *, int, short *,
                                           CORBA_Server_Environment *);
    // points to the real reply function
    void               (*rx_reply_func)(l4ore_handle_t);
    // points to the netif_rx function
    int                (*netif_rx_func)(l4ore_handle_t, struct sk_buff *);
    // points to the real tx component function
    int                (*tx_component_func)(CORBA_Object,
                                            l4ore_handle_t,
                                            const char *, l4_umword_t,
                                            int, short *,
                                            CORBA_Server_Environment *);
} ore_connection_t;

/* ORe connection table */
ore_connection_t ore_connection_table[ORE_CONFIG_MAX_CONNECTIONS];
/* the main server thread */
l4_threadid_t ore_main_server;

/* lock to secure the connection table ==> OBSOLETE!!! */
//l4lock_t ore_connection_table_lock;

/* Connection handling */
void init_connection_table(void);           // initialize conn table
l4ore_handle_t getUnusedConnection(void);   // get unused
int setup_connection(char *device_name, unsigned char mac[6], // init
                     const l4dm_dataspace_t *send_ds,
                     const l4dm_dataspace_t *recv_ds,
                     unsigned char mac_address_head[4],
                     l4ore_config *conf,
                     l4ore_handle_t handle);
int free_connection(l4ore_handle_t handle); // free conn
// and device fitting skb

/* rx/tx entry management */
void free_rxtx_entry(rxtx_entry_t *e);
void clear_rxtx_list(struct list_head *h);

/* rx/tx functions for the string ipc case */
int rx_component_string(CORBA_Object, l4ore_handle_t, char **, l4_umword_t *,
                        l4_umword_t *, int, short *,
                        CORBA_Server_Environment *);
void rx_to_client_string(l4ore_handle_t channel);
int netif_rx_string(l4ore_handle_t, struct sk_buff *);
int tx_component_string(CORBA_Object, l4ore_handle_t, const char *,
                        l4_umword_t, int, short *,
                        CORBA_Server_Environment *);

/* rx/tx functions for the shared mem case
 *
 * rx_component_shmem();
 * tx_component_shmem();
 * netif_rx_shmem();
 * rx_to_client_shmem();
 */

/* IRQ handling */
extern void custom_irq_handler(l4_threadid_t, l4_umword_t, l4_umword_t);
extern void irq_handler(l4_int32_t irq, void *arg);

/* netdevice handling */
extern l4_int32_t list_network_devices(void);
extern l4_int32_t open_network_devices(void);

/* utility functions */
int mac_equal(ore_mac mac1, ore_mac mac2);
int mac_is_broadcast(ore_mac mac);
l4ore_handle_t find_channel_for_skb(struct sk_buff *skb, int start);
l4ore_handle_t find_channel_for_mac(ore_mac mac);
int local_deliver(rxtx_entry_t *);

#endif /* ! __ORE_LOCAL_H */
