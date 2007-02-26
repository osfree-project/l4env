#ifndef __DICE_SOCKETS_H__
#define __DICE_SOCKETS_H__

/* Socket specific includes */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#ifndef DICE_DEFAULT_PORT
#define DICE_DEFAULT_PORT (in_port_t)9999
#endif

#define dice_default_environment { CORBA_NO_EXCEPTION, \
    0, 0, (in_port_t)9999, -1 , 0, \
    malloc, free, \
    { sin_family: 0, sin_port: 0, sin_addr: { s_addr: 0 } }, \
    { 0,0,0,0,0, 0,0,0,0,0}, 0, { 0L, 0L } }
#define dice_default_server_environment dice_default_environment

#ifdef __cplusplus
namespace dice
{   
    CORBA_Environment::CORBA_Environment()
    : exc_major(0),
      repos_id(0),
      param(0),
      srv_port(9999),
      cur_socket(-1),
      user_data(0),
      partner(),
      malloc(::malloc),
      free(::free),
      ptrs_cur(0),
      receive_timeout()
    {
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = 0;
    }
}
#endif

#endif
