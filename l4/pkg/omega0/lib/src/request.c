#include <l4/omega0/client.h>
#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include "internal.h"
#include "config.h"

omega0_alien_handler_t omega0_alien_handler;

/* do an irq-related request. */
int omega0_request(int handle, omega0_request_t action){
  if(!omega0_initalized && omega0_init()) return -1;
  
  if(omega0_alien_handler){
    l4_threadid_t alien;
    l4_umword_t d0, d1;
    int err;
    
    while(1) {
      if((err = omega0_open_call(handle, OMEGA0_REQUEST, action.i,
                                 L4_IPC_NEVER, &alien, &d0, &d1))!=0) 
	return err;
      if(!l4_task_equal(alien, omega0_management_thread)){
        omega0_alien_handler(alien, d0, d1);
      } else return d0;
      action.s.consume=action.s.mask=action.s.unmask=0;
      action.s.again=1;
    }
  } else
  return omega0_call(handle, OMEGA0_REQUEST, action.i, L4_IPC_NEVER);
}

omega0_alien_handler_t omega0_set_alien_handler(
    omega0_alien_handler_t handler){
  omega0_alien_handler_t old = omega0_alien_handler;
  omega0_alien_handler = handler;
  return old;
}
