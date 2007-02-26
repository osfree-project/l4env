#include <l4/sys/ipc.h>
#include <l4/omega0/client.h>
#include <omega0_proto.h>
#include "internal.h"
#include "config.h"

/* detach from a certain irq line. */

int omega0_detach(omega0_irqdesc_t desc){
  if(!omega0_initalized && omega0_init()) return -1;
  
  return omega0_call(MANAGEMENT_THREAD, OMEGA0_DETACH, desc.i, L4_IPC_NEVER);
}
