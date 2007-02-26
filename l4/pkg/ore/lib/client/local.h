#ifndef _LOCAL_H_
#define _LOCAL_H_

#include <l4/ore/ore.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/env/errno.h>
#include "ore-client.h"

// someone needs to store who ORe is
extern l4_threadid_t ore_server;

// maximum no. of connections one client may have
#define CONN_MAX 8

typedef struct ore_client_conn_desc{
    int (*send_func)(l4ore_handle_t, char *, unsigned int);
    int (*rx_func_blocking)(l4ore_handle_t, char **, unsigned int *);
    int (*rx_func_nonblocking)(l4ore_handle_t, char **, unsigned int *);
} ore_client_conn_desc;

ore_client_conn_desc descriptor_table[CONN_MAX];

int ore_lookup_server(void);
l4ore_handle_t ore_do_open(const char *dev, unsigned char mac[6],
                           l4dm_dataspace_t *send, l4dm_dataspace_t *recv,
                           l4ore_config *flags);
int ore_send_string(l4ore_handle_t channel, char *data, unsigned int size);
int ore_recv_string_blocking(l4ore_handle_t channel, char **data, unsigned int *size);
int ore_recv_string_nonblocking(l4ore_handle_t channel, char **data, unsigned int *size);
void ore_do_close(l4ore_handle_t handle);

#endif //_LOCAL_H_
