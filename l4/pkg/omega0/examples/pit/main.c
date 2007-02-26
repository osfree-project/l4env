#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/omega0/client.h>
#include <l4/sys/kdebug.h>
#include <l4/util/spin.h>
#include <l4/util/rdtsc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/ipc.h>
#include "config.h"
#include "pit.h"
#include "pic.h"

static int attach(int irq, int*handle){
#ifdef USE_OMEGA0
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

#else	// non-omega0

  l4_threadid_t irq_th;
  l4_umword_t dummy;
  l4_msgdope_t result;
  int error;
  static int old_irq = -1;

  if(old_irq!=-1){
    if(old_irq!=irq) return 1;
    *handle = irq+1;
    return 0;
  }

  if(rmgr_get_irq(irq)) return 2;
  l4_make_taskid_from_irq(irq, &irq_th);
  
  error = l4_ipc_receive(irq_th, 0, &dummy, &dummy,
                         L4_IPC_BOTH_TIMEOUT_0, &result);

  if(error!=L4_IPC_RETIMEOUT) return 3;
  *handle = irq+1;
  old_irq = irq;
  return 0;
#endif
}

static int detach(int irq){
#ifdef USE_OMEGA0
  omega0_irqdesc_t desc;
  int err;

  desc.s.shared = 0;
  desc.s.num = irq+1;
  
  if((err = omega0_detach(desc))<0){
    LOGl("error %d detaching from irq %x", err, irq);
    return 1;
  }
  return 0;
#else
  return 0;	// dont detach
#endif
}

static int irq_request(int handle, omega0_request_t request){
#ifdef USE_OMEGA0
  return omega0_request(handle, request);

#else	// non-omega0

  l4_threadid_t irq_th;
  int err;
  l4_umword_t dummy;
  l4_msgdope_t result;
  
  if(request.s.consume){
    irq_unmask(request.s.param-1);
  }
  if(request.s.unmask){
    irq_unmask(request.s.param-1);
  }
  if(request.s.wait){
    l4_make_taskid_from_irq(handle-1, &irq_th);
    err = l4_ipc_receive(irq_th, L4_IPC_SHORT_MSG, &dummy, &dummy,
                              L4_IPC_NEVER, &result);
    irq_mask(handle-1);
    irq_ack(handle-1);
    if(err) return err;
  }
  return 0;
#endif
}

static void pit(int freq, int time){
  int handle;
  omega0_request_t request;
  int err;
  l4_umword_t count = 0;
  l4_cpu_time_t until;

  if((err=attach(0, &handle))!=0){
    LOGl("error %d attaching to irq %x", err, 0);
    enter_kdebug("!");
    return;
  }
  
  pit_set_freq(freq);
  request.s.unmask = 1;
  request.s.mask = 0;
  request.s.consume = 0;
  request.s.wait = 1;
  request.s.param = 0+1;
  
  until = l4_rdtsc() + l4_ns_to_tsc(time*1000*1000*1000LL);
  
  for(count=0;;count++){
    err = irq_request(handle, request);
    if(err<0){
      LOGl("omega0_request(handle=%d, request=%p) returned %d",
                           handle, &request, err);
      enter_kdebug("!");
      continue;
    }
    request.s.consume = 1;
    request.s.unmask = 0;
    //l4_spin_text(0,13,"irq 0: ");
    
    if( l4_rdtsc() >= until) break;
  }
  LOG("in %d secs I got %lu/%u irqs (%ld%%), freq was %u",
      time, count, freq*time, count*100/(freq*time), freq);
  detach(0);
}

int main(int argc, char*argv[]){
  rmgr_init();
  if(l4_tsc_init(L4_TSC_INIT_KERNEL)==0){
      LOG("Error getting TSC/CPU scalers from kernel.");
      return 1;
  }
#ifdef USE_OMEGA0
  LOG("Using Omeag0 server.");
#else
  LOG("Programmin interrupts myself.");
#endif
  
  pit(20000, 2);
  pit(50000, 2);
  pit(100000, 2);
  pit(200000, 2);
  pit(300000, 2);
  pit(400000, 2);

  pit(500000, 2);
  pit(1000000, 2);
  
  enter_kdebug("ready.");
  LOG("falling asleep");
  
  while(1)l4_sleep(1000);

  return 0;
}
