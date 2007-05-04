/****************************************************************
 * ORe local declarations.										*
 *																*
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>					*
 * 2005-08-10													*
 *                                                              *
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef __ORE_LOCAL_H
#define __ORE_LOCAL_H

#include <l4/log/l4log.h>
#include <l4/ore/ore.h>
#include <l4/dde_linux/dde.h>
#include <l4/dm_generic/types.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>
#include <dice/dice.h>

#include "config.h"
#ifndef CONFIG_ORE_DDE26
#include "linuxemul.h"
#endif
#include "auto_config.h"
#include <l4/ore/worker-server.h>
#include <l4/ore/ore_manager-server.h>
#include <l4/ore/ore_rxtx-server.h>
#include <l4/ore/ore_notify-server.h>
#include <l4/ore/ore_notify-client.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>

/** Assert that an expression is true and panic if not. */
#define Assert(expr)	do 									\
	{														\
		if (!expr) {										\
			LOG_printf("\033[31;1mAssertion failed: "#expr"\033[0m\n");	\
			LOG_printf("  File: %s:%d\n",__FILE__,__LINE__); 		\
			LOG_printf("  Function: %s()\n", __FUNCTION__);	\
			enter_kdebug("Assertion failed.");				\
		} \
	} while (0);

/* this is the kind of data we are going to store in the rx
 * and tx lists */
typedef struct rxtx_entry_t
{
    struct list_head list;     		// the list we are on
    struct sk_buff   *skb;     		// socket buffer
    int				 in_dataspace; 	// entry points to a dataspace
} rxtx_entry_t;

/* All the information we need about a DSI packet */
typedef struct ore_dsi_desc
{
	struct list_head 	list;		// list
	void				*addr;		// start address of data area
	l4_size_t			size;		// size
	dsi_packet_t 		*packet;	// packet
	dsi_socket_t		*socket;	// socket
} ore_dsi_desc;

typedef unsigned char ore_mac[6];

/* connection state used by ORe */
typedef struct ore_connection_t{
    int               in_use;         // connection in use?
    l4_threadid_t     owner;          // client for this connection
    l4_threadid_t     worker;         // worker thread for this connection
    l4_threadid_t     worker_dsi;     // dsi worker thread
    struct net_device *dev;           // opened device
    ore_mac           mac;            // allocated MAC address
    
    // TODO: union{
    struct list_head  rx_list;        // list for rx packets (string ipc)
    dsi_socket_t      *rx_socket;     // socket for sending rx packets to DSI client
    // }
    void              *rx_start;      // start address of the recv dataspace
    void              *rx_addr;       // next free address in recv dataspace
    l4_size_t         rx_size;        // size of the recv dataspace
    // TODO: union{
    struct list_head  tx_list;        // list for tx packets 
    dsi_socket_t      *tx_socket;     // socket for receiving send packets from DSI client
    // }
    l4lock_t          tx_startlock;
    
    l4lock_t          channel_lock;   // channel lock
    l4_int32_t        flags;          // server-side connection flags
    l4ore_config      config;         // client-side configuration
    l4_threadid_t     waiting_client; // currently waiting client
    int               waiting_size;   // buffer size of the currently waiting client

    int               packets_received;     // packets received by netif_rx() so far
    int               packets_sent;         // packets sent so far by the client
    int               packets_queued;       // packets in queue
    
    // points to the real rx component function
    int               (*rx_component_func)(CORBA_Object, char **, l4_size_t,
                                           l4_size_t *, int, short *,
                                           CORBA_Server_Environment *);
    // points to the real reply function
    void               (*rx_reply_func)(int);
    // points to the netif_rx function
    int                (*netif_rx_func)(int , struct sk_buff *);
    // points to the real tx component function
    int                (*tx_component_func)(CORBA_Object,
                                            const char *, l4_size_t,
                                            CORBA_Server_Environment *);
} ore_connection_t;

/* ORe connection table */
extern ore_connection_t ore_connection_table[ORE_CONFIG_MAX_CONNECTIONS];
/* the main server thread */
extern l4_threadid_t ore_main_server;
/* local storage key for a worker thread's channel id */
extern int __l4ore_tls_id_key;
/* set to 1 if we only want to use the loopback device */
extern int loopback_only;

/* Connection handling */
void init_connection_table(void);           // initialize conn table
int getUnusedConnection(void);   // get unused
int setup_connection(char *device_name, unsigned char mac[6], // init
                     unsigned char mac_address_head[4],
                     l4ore_config *conf,
                     int channel,
                     l4_threadid_t *owner);
int free_connection(int channel); // free conn

/* rx/tx entry management */
void free_rxtx_entry(rxtx_entry_t *e);
void clear_rxtx_list(struct list_head *h);

/* rx/tx functions for the string ipc case */
int rx_component_string(CORBA_Object, char **, l4_size_t,
                        l4_size_t *, int, short *,
                        CORBA_Server_Environment *);
void rx_to_client_string(int channel);
int netif_rx_string(int , struct sk_buff *);
int tx_component_string(CORBA_Object, const char *,
                        l4_size_t, CORBA_Server_Environment *);

int netif_rx_dsi(int , struct sk_buff *);

/* IRQ handling */
extern void custom_irq_handler(l4_threadid_t, l4_umword_t, l4_umword_t);
extern void irq_handler(l4_int32_t irq, void *arg);

/* netdevice handling */
extern l4_int32_t list_network_devices(void);
extern l4_int32_t open_network_devices(void);

/* in a multithreaded server we need to lock hard_start_xmit for every
 * device
 */
int xmit_lock(char *dev);               
int xmit_unlock(char *dev);             
int xmit_lock_add(char *dev);           
int xmit_lock_remove(char *dev); 

/* utility functions */
int mac_equal(ore_mac mac1, ore_mac mac2);
int mac_is_broadcast(ore_mac mac);
int find_channel_for_skb(struct sk_buff *skb, int start);
int find_channel_for_mac(ore_mac mac, int start);
int find_channel_for_threadid(l4_threadid_t, int);
int find_channel_for_worker(l4ore_handle_t worker);
int local_deliver(rxtx_entry_t *, int channel);
int service_waiting_clients(void);
int sanity_check_rxtx(int, l4_threadid_t);
int __l4ore_in_dataspace(void *, dsi_packet_t **, dsi_socket_t **);
void __l4ore_do_packet_commit(dsi_packet_t *, dsi_socket_t *);
void __l4ore_remember_packet(dsi_packet_t *, dsi_socket_t *, void *, 
	l4_size_t);
int init_dsi_sendingclient(int channel, l4ore_config *conf);
int init_dsi_receivingclient(int channel, l4ore_config *conf);
    

/* thread for handling external events */
void handle_events(void *);
/* worker thread functions */
void worker_thread_string(void *);
void worker_thread_dsi(void *);

/* checksum functions */
unsigned int adler32(unsigned char *buf, unsigned int len);

#ifndef CONFIG_ORE_DDE26
unsigned short crc16(unsigned char *buf, int len, short magic);
unsigned int crc32(unsigned char *buf, int len, short magic);
#else
#include <linux/crc16.h>
#include <linux/crc32.h>
#endif

/* unit test function */
extern void cunit_tests(void);

/* debugging stuff */
#ifdef CONFIG_ORE_DUMPER
void dump_periodic(void *);
#endif

#endif /* ! __ORE_LOCAL_H */
