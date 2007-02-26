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
#include <stdlib.h>

#ifndef DICE_DEFAULT_PORT
#define DICE_DEFAULT_PORT (in_port_t)9999
#endif

#include "dice/dice-env_functions.h"

#define DICE_REPLY    1
#define DICE_NO_REPLY 2

#define dice_default_environment \
  { CORBA_NO_EXCEPTION, 0, 0, 9999, -1 , 0, \
    malloc, free }
#define dice_default_server_environment dice_default_environment

#endif
