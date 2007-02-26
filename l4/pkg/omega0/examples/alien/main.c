/*!
 * \file   omega0/examples/alien/main.c
 * \brief  Alien demo: Thread sending IPC to an IRQ-waiting thread
 *
 * \date   10/06/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/omega0/client.h>
#include <l4/sys/kdebug.h>
#include <l4/util/spin.h>
#include <l4/util/rdtsc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/ipc.h>
#include <l4/util/macros.h>
#include <l4/util/thread.h>
#include "config.h"
#include "pit.h"
#include "pic.h"

static l4_threadid_t main_thread;

static int attach(int irq, int*handle){
    omega0_irqdesc_t desc;

    if(!handle){
	LOGl("error: given handle points to %p", handle);
	return 1;
    }
  
    desc.s.shared = 0;
    desc.s.num = irq+1;
  
    if((*handle=omega0_attach(desc))<0){
	LOGl("error %#x attaching to irq %x", *handle, irq);
	return 1;
    }
    return 0;

}

static int detach(int irq) __attribute__((unused));
static int detach(int irq){
    omega0_irqdesc_t desc;
    int err;

    desc.s.shared = 0;
    desc.s.num = irq+1;
  
    if((err = omega0_detach(desc))<0){
	LOGl("error %d detaching from irq %x", err, irq);
	return 1;
    }
    return 0;
}

static void alien_handler(l4_threadid_t alien, l4_umword_t d0,
			  l4_umword_t d1){
    l4_spin_text_vga(0,14,"alien:   ");
}


static void pit(int freq){
    int handle;
    omega0_request_t request;
    int err;
    l4_umword_t count = 0;

    if((err=attach(0, &handle))!=0){
	LOGl("error %d attaching to irq %x", err, 0);
	enter_kdebug("!");
	return;
    }
  
    pit_set_freq(freq);
    request.i = 0;
    request.s.unmask = 1;
    request.s.mask = 0;
    request.s.consume = 0;
    request.s.wait = 1;
    request.s.param = 0+1;
  
    for(count=0;;count++){
	err = omega0_request(handle, request);
	if(err<0){
	    LOGl("omega0_request(handle=%d, request=%p) returned %d",
		 handle, &request, err);
	    enter_kdebug("!");
	    continue;
	}
	request.s.consume = 1;
	request.s.unmask = 0;
	l4_spin_text_vga(0,13,"irq 0:  ");
    
    }
}

static int stack[4096];
static void ipc_thread(void){
    int i;
    l4_msgdope_t result;

    for(i=0;;i++){
	l4_ipc_send(main_thread, L4_IPC_SHORT_MSG, i++, 0,
		    L4_IPC_NEVER, &result);
	l4_sleep(10);
    }
}

int main(int argc, char*argv[]){
    rmgr_init();
    main_thread = l4_myself();
    l4util_create_thread(1, ipc_thread, stack+sizeof(stack));
    
    omega0_set_alien_handler(alien_handler);
    
    pit(50000);
    
    while(1)l4_sleep_forever();
}
