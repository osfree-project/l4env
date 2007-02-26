#include "uip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <arpa/inet.h>

#include <l4/sys/kdebug.h>
#include <l4/util/macros.h>
#include <l4/ore/uip-ore.h>

struct conn_desc conn_table[MAX_CONN];

/* enqueue a packet to be sent by uip_ore */
void uip_ore_send(const char *buf, unsigned size, unsigned port)
{
    // construct a list entry from buf and size
    struct rxtx_entry *e = malloc(sizeof(struct rxtx_entry));
    short idx = get_id_for_port(port);

    if (idx < 0)
    {
        printf("No such port: %d\n", port);
        return;
    }

    if (!e)
    {
        PANIC("Out of memory!\n");
        return;
    }
    
    e->buf = (typeof(e->buf))buf;
    e->size = size;
    
    // enqueue to send list
    l4lock_lock(&conn_table[idx].__listlock);
    list_add_tail(&e->list, &conn_table[idx].__tx_list); 
    l4lock_unlock(&conn_table[idx].__listlock);
}

/* connect to ip:port */
int uip_ore_connect(struct in_addr ip, unsigned port)
{
    unsigned short uip_ip[2];

    printf("connecting to %s:%hu\n", inet_ntoa(ip), port);
    // copy the IP
    memcpy(uip_ip, &ip, 4);
    uip_connect(uip_ip, HTONS(port));
    return 0;
}

void uip_ore_close(unsigned port)
{
    short idx = get_id_for_port(port);
    if (idx >= 0 && idx < MAX_CONN)
        conn_table[idx].close_flag = 1;
}

