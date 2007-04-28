#include <l4/omega0/client.h>
#include <omega0_proto.h>
#include "internal.h"
#include "config.h"

/* pass the right to attach to a certain irq line to another thread. */
int omega0_pass(omega0_irqdesc_t desc, l4_threadid_t new_driver){
  if(!omega0_initalized && omega0_init()) return -1;
  
  return omega0_call_long(0, OMEGA0_PASS, 
                          desc.i, new_driver);
}
