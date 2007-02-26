#ifndef __UIP_ORE_LOCAL_H
#define __UIP_ORE_LOCAL_H

#include <l4/lock/lock.h>
#include <l4/log/l4log.h>
#include "list.h"
#include "uip.h"
#include "oredev.h"

#define MAX_CONN 5

// send list entry
struct rxtx_entry
{
    struct list_head list;
    void *buf;
    l4_size_t size;
};

// connection descriptor
struct conn_desc
{
    unsigned port;
    struct list_head __tx_list;
    l4lock_t __listlock;
    int close_flag;
};

extern struct conn_desc conn_table[MAX_CONN];
extern struct uip_ore_config __uip_ore_config;

// uIP connection state
struct uip_connstate
{
    unsigned short state;
};

void uip_ore_callback(void);

// find the connection id for a given port
short get_id_for_port(unsigned port);
// add connection to port if there is a free left,
// return -1 otherwise
short add_port(unsigned port);
// clean up all connection info
void cleanup_connection(unsigned port);

#define UIP_READY   1
#define UIP_BUSY    2

#define UIP_APPSTATE_SIZE   (sizeof(struct uip_connstate))
#define UIP_APPCALL         uip_ore_callback

#endif
