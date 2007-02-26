/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef __LOG_ORE_H
#define __LOG_ORE_H

#include "uip.h"
#include "list.h"
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/lock/lock.h>

#define LOG_ORE_WAIT_FOR_CLIENT     1
#define LOG_ORE_CONNECTED           2

#define LOG_READY                   0
#define WAIT_ACK                    1

// connection state
struct log_state
{
    unsigned short  state;
};

// elements to be put into the rx/tx lists
struct rxtx_entry
{
    struct list_head list;
    char *buf;
    l4_size_t size;
};

#define UIP_APPSTATE_SIZE   (sizeof(struct log_state))
#define UIP_APPCALL         log_ore_callback

extern unsigned client_socket;

void log_ore_init(void);
void log_ore_callback(void);
void log_ore_threadfunc(void *p);

void log_ore_send_to_channel(void *addr, int size, int channel);
void log_ore_flush_buffer(void);

// config options
extern char *ip_addr;
extern int port_nr;
extern int debug;
extern int log_kdebug;
extern int binary;

extern l4_threadid_t log_ore_thread;
extern l4lock_t __log_ore_rxtx_lock;
extern struct list_head __log_ore_rx_list, __log_ore_tx_list;

#endif
