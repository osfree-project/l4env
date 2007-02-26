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

#ifndef DICE_DEFAULT_PORT
#define DICE_DEFAULT_PORT (in_port_t)9999
#endif

#define dice_default_environment \
  { CORBA_NO_EXCEPTION, 0, 0, 9999, -1 , 0, \
    malloc, free, \
    { 0,0,0,0,0, 0,0,0,0,0}, 0 }
#define dice_default_server_environment dice_default_environment

#ifdef __cplusplus
namespace dice
{   
    CORBA_Environment::CORBA_Environment()
    {
	major = 0;
	repos_id = 0;
	param = 0;
	srv_port = 9999;
	cur_socket = -1;
	user_data = 0;
	this->malloc = ::malloc;
	this->free = ::free;
	
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = 0;
	ptrs_cur = 0;
    }
}
#endif

#endif
