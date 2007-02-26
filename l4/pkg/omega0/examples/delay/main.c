#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/omega0/client.h>
#include <l4/sys/kdebug.h>
#include <l4/util/spin.h>
#include <l4/util/rdtsc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/ipc.h>
#include <stdio.h>
#include "config.h"
#include "serial.h"
#include "pic.h"

char LOG_tag[9]="pit";

#define MHZ 400000000	// 1Mio*400
#define COUNT 100

int port = 0x3f8;      // com1
int irq  = 4;          // irq 4

extern void outb(int port, unsigned char val);
extern unsigned char inb(int port);

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
  l4_uint32_t dummy;
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
  l4_uint32_t dummy;
  l4_msgdope_t result;
  
  if(request.s.consume){
    irq_unmask(request.s.param-1);
  }
  if(request.s.unmask){
    irq_unmask(request.s.param-1);
  }
  if(request.s.mask){
    irq_mask(request.s.param-1);
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

unsigned long long time_field[COUNT];

static void measure_delay(void){
  int handle;
  int err;
  l4_uint32_t count = 0;
  unsigned long long sum=0, t0, t1;
  omega0_request_t request;

  if((err=attach(irq, &handle))!=0){
    LOGl("error %d attaching to irq %x", err, irq);
    enter_kdebug("!");
    return;
  }
  //LOGl("got irq 0");
  enter_kdebug(".");
  
  request = OMEGA0_RQ(OMEGA0_WAIT|OMEGA0_UNMASK, irq+1);
  
  for(count=0;count<=COUNT;count++){
    /* generate an irq, pic is in masked state */
    serial_send_char(port, 'x');
    l4_sleep(40);	// 10ms should be enough, 9 bit at 115Kbit/s needs
                        // .07ms
                        
    // unmask and get the irq
    t0 = l4_rdtsc();
    //err = irq_request(handle, OMEGA0_RQ(OMEGA0_UNMASK|OMEGA0_WAIT, irq+1));
    err = irq_request(handle, request);
    t1 = l4_rdtsc();
    if(err<0){
      LOGl("omega0_request(handle=%d, request=%#x) returned %d",
                           handle, request.i, err);
      enter_kdebug("!");
      continue;
    }
    time_field[count]=t1-t0;
    sum+=t1-t0;
    request.s.unmask= 0;
    request.s.consume=1;
    
#if 0
    // consume and mask the irq
    err = irq_request(handle, OMEGA0_RQ(OMEGA0_CONSUME,irq+1));
    if(err<0){
      LOGl("omega0_request(handle=%d, request=%#x) returned %d",
            handle, OMEGA0_RQ(OMEGA0_CONSUME|OMEGA0_MASK,irq+1).i,
                           err);
      enter_kdebug("!");
      continue;
    }
#endif
  }
  LOG("I got %u irqs, needed %u cycles per irq. This are:", 
      COUNT, (unsigned)(sum/COUNT));
  for(count=0;count<COUNT;count++){
    printf("%u ",(unsigned)time_field[count]);
  }
  irq_request(handle, OMEGA0_RQ(OMEGA0_UNMASK, irq+1));
  detach(irq);
}

int main(int argc, char*argv[]){
  rmgr_init();
  
  #ifdef USE_OMEGA0
    LOG("using Omega0-server");
  #else
    LOG("using native irq-handling.\n");
    #ifdef USE_LOCKING
      LOG(" + use locking\n");
    #else
      LOG(" + do not use locking\n");
    #endif
    #ifdef USE_CLISTI
      LOG(" + using CLI/STI-pairs around PIC manipulation\n");
    #else
      LOG(" + do not use cli/sti-pairs on PIC access\n");
    #endif
  #endif
  
  serial_init(port, irq);
  l4_sleep(1000);
  measure_delay();

  enter_kdebug("ready.");
  LOG("falling asleep");
  
  while(1){
    l4_sleep(1000);
  }
  return 1;
}
