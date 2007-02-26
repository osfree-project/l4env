#include <l4/omega0/client.h>
#include <omega0_proto.h>
#include "internal.h"
#include "config.h"

/* do an irq-related request. */

int omega0_request(int handle, omega0_request_t action){
  if(!omega0_initalized && omega0_init()) return -1;
  
  return omega0_call(handle, OMEGA0_REQUEST, action.i);
}
