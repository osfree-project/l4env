#ifndef __DICE_SOCKETS_H__
#define __DICE_SOCKETS_H__

#include "dice/dice-common.h"

/* Socket specific includes */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>

#ifndef CORBA_Object_typedef
#define CORBA_Object_typedef
typedef struct sockaddr_in CORBA_Object;
#endif

#include "dice/dice-env_types.h"

#ifndef CORBA_Environment_typedef
#define CORBA_Environment_typedef
typedef struct CORBA_Environment_t
{
  COMMON_ENVIRONMENT;
  in_port_t srv_port;
  int cur_socket;
  void* user_data;
} CORBA_Environment;
#endif

#ifndef DICE_DEFAULT_PORT
#define DICE_DEFAULT_PORT (in_port_t)9999
#endif

#include "dice/dice-env_functions.h"

#define dice_default_environment { CORBA_NO_EXCEPTION, 0, 0, 9999, -1 , 0 }

#endif
