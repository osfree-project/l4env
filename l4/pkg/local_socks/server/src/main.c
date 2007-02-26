/* *** GENERAL INCLUDES *** */
#include <stdio.h>

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/names/libnames.h>

/* *** LOCAL INCLUDES *** */
#include "local_socks-server.h"
#include "socket_internal.h"

int main(int argc, char**argv){

  names_register("PF_LOCAL");
  local_socks_init();
  local_socks_server_loop(NULL);
  
  return 0;
}
