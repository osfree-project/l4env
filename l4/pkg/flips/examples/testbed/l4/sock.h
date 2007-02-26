#ifndef __MY_SOCK_H
#define __MY_SOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <l4/flips/libflips.h>

#define ERR_EXIT(str) \
  do {                \
    perror(str);      \
    return 1;         \
  } while (0)

#define socket     flips_socket
#define connect    flips_connect
#define recv       flips_recv
#define setsockopt flips_setsockopt  /* NIY */
#define bind       flips_bind
#define listen     flips_listen
#define accept     flips_accept
#define shutdown   flips_shutdown

#define read       flips_read
#define write      flips_write

#endif

