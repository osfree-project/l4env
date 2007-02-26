/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

/* Utility functions necessary to handle send/receive through multiple ports. */

#include "uip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <arpa/inet.h>

#include <l4/sys/kdebug.h>
#include <l4/util/macros.h>
#include <l4/ore/uip-ore.h>

void cleanup_connection(unsigned port)
{
    struct list_head *p, *n;
    struct list_head *h;
    int i = get_id_for_port(port);

    // nothing left to cleanup!
    if (i < 0)
        return;
    
    conn_table[i].port = 0;
    
    if (list_empty(&conn_table[i].__tx_list))
            return;

    h = &conn_table[i].__tx_list;
    list_for_each_safe(p, n, h)
    {
        struct rxtx_entry *ent = list_entry(p, struct rxtx_entry, list);
        if (!ent)
            continue;
        if (__uip_ore_config.ack_callback != NULL)
            __uip_ore_config.ack_callback(ent->buf, port);
        list_del(p);
        free(ent);
    }
}

/* Add a port mapping to the connection table.
 *
 * \param port  port
 * \retval  -1  out of connections
 *          0   connetion added
 *          >0  connection already there
 */
short add_port(unsigned port)
{
    int i;
    for (i=0; i<MAX_CONN; i++)
    {
        // already there?
        if (conn_table[i].port == port)
            return i+1;
        if (conn_table[i].port == 0)
            break;
    }

    // out of connections
    if (i >= MAX_CONN)
        return -1;

    // setup
    conn_table[i].port = port;
    conn_table[i].__listlock = L4LOCK_UNLOCKED;
    conn_table[i].close_flag = 0;
    INIT_LIST_HEAD(&conn_table[i].__tx_list);
    return 0;
}

short get_id_for_port(unsigned port)
{
    int i;
    for (i=0; i<MAX_CONN; i++)
    {
        if (conn_table[i].port == port)
            return i;
    }
    return -1;
}
