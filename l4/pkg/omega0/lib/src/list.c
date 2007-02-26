#include <l4/sys/ipc.h>
#include <l4/omega0/client.h>
#include <omega0_proto.h>
#include "internal.h"
#include "config.h"

/* list the available irq lines. */

int omega0_first(void){
  if(!omega0_initalized && omega0_init()) return -1;
  
  return omega0_call(MANAGEMENT_THREAD, OMEGA0_FIRST, 0, L4_IPC_NEVER);
}

int omega0_next(int num){
  if(!omega0_initalized && omega0_init()) return -1;
  
  return omega0_call(MANAGEMENT_THREAD, OMEGA0_NEXT, num, L4_IPC_NEVER);
}
