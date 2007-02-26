#ifndef _LOCAL_H_
#define _LOCAL_H_

#include <l4/ore/ore.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/env/errno.h>
#include <l4/ore/ore-dsi.h>
#include <l4/dm_mem/dm_mem.h>
#include <stdlib.h>
#include "ore_rxtx-client.h"
#include "ore_manager-client.h"

// maximum no. of connections one client may have
#define CONN_MAX 8

typedef struct ore_client_conn_desc{
	l4ore_handle_t remote_manager_thread;	// manager of the remote ORe instance
    l4ore_handle_t remote_worker_thread;    // ORe worker
    dsi_socket_t *local_send_socket;        // DSI send socket
    dsi_socket_t *local_recv_socket;        // DSI rx socket
    void *send_addr;                        // address of send DS
    void *recv_addr;                        // start of recv DS
    int (*send_func)(l4ore_handle_t, int, char *, l4_size_t);
    int (*rx_func_blocking)(l4ore_handle_t, int, char **, l4_size_t *, l4_timeout_t);
    int (*rx_func_nonblocking)(l4ore_handle_t, int, char **, l4_size_t *);
} ore_client_conn_desc;

extern ore_client_conn_desc descriptor_table[CONN_MAX];
extern int ore_initialized;

int ore_lookup_server(char *orename, l4ore_handle_t *manager);
l4ore_handle_t ore_do_open(int handle, const char *dev, 
		                   unsigned char mac[6], l4ore_config *flags);

// string ipc functions
int ore_send_string(l4ore_handle_t channel, int handle, char *data, l4_size_t size);
int ore_recv_string_blocking(l4ore_handle_t channel, int handle, char **data, l4_size_t *size, l4_timeout_t);
int ore_recv_string_nonblocking(l4ore_handle_t channel, int handle, char **data, l4_size_t *size);

// dsi shared memory functions
int ore_send_dsi(l4ore_handle_t channel, int handle, char *data, l4_size_t size);
int ore_recv_dsi_blocking(l4ore_handle_t channel, int handle, char **data, l4_size_t *size, l4_timeout_t);
int ore_recv_dsi_nonblocking(l4ore_handle_t channel, int handle, char **data, l4_size_t *size);

int __l4ore_init_send_socket(l4ore_handle_t, l4ore_config *conf, dsi_socket_t **, void **); 
int __l4ore_init_recv_socket(l4ore_handle_t, l4ore_config *conf, dsi_socket_t **, void **); 
void __l4ore_remember_packet(dsi_socket_t *, dsi_packet_t *, void *, l4_size_t);

void ore_do_close(int handle);

#endif //_LOCAL_H_
