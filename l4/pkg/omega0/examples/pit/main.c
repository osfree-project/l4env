#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/omega0/client.h>
#include <l4/sys/kdebug.h>
#include <l4/util/spin.h>
#include <l4/util/rdtsc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/ipc.h>

/* define USE_OMEGA0 if you want to use omega0-server. Dont define it to
   compare against native implementation.
*/
#define USE_OMEGA0

#define MHZ 100000000	// 1Mio*100

extern void outb(int port, unsigned char val);
extern unsigned char inb(int port);
void pit_set_freq(int);
int attach(int, int*);
void pit(int freq, int time);

int attach(int irq, int*handle){
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
  irq_th.lh.low = irq + 1;
  irq_th.lh.high = 0;
  
  error = l4_i386_ipc_receive(irq_th, 0, &dummy, &dummy,
                              L4_IPC_TIMEOUT(0,1,0,1,0,0), &result);

  if(error!=L4_IPC_RETIMEOUT) return 3;
  *handle = irq+1;
  old_irq = irq;
  return 0;
#endif
}

int detach(int irq);
int detach(int irq){
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

int irq_request(int handle, omega0_request_t request);
int irq_request(int handle, omega0_request_t request){
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
    irq_th.lh.low = handle;
    irq_th.lh.high = 0;
    err = l4_i386_ipc_receive(irq_th, L4_IPC_SHORT_MSG, &dummy, &dummy,
                              L4_IPC_NEVER, &result);
    irq_mask(handle-1);
    irq_ack(handle-1);
    if(err) return err;
  }
  return 0;
#endif
}

void pit(int freq, int time){
  int handle;
  omega0_request_t request;
  int err;
  l4_umword_t count = 0;
  unsigned long long until;

  if((err=attach(0, &handle))!=0){
    LOGl("error %d attaching to irq %x", err, 0);
    enter_kdebug("!");
    return;
  }
  LOGl("got irq 0");
  
  pit_set_freq(freq);
  request.s.unmask = 1;
  request.s.mask = 0;
  request.s.consume = 0;
  request.s.wait = 1;
  request.s.param = 0+1;
  
  until = l4_rdtsc() + MHZ*time;	// time secs
  
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
  LOG("in %d secs I got %u irqs, freq was %u", time, count, freq);
  detach(0);
}

void start_thread(int thread, void*function, void*stack, int port, int irq);
void start_thread(int thread, void*function, void*stack, int port, int irq){
  l4_umword_t dummy;
  l4_threadid_t pager=L4_INVALID_ID, preempter=L4_INVALID_ID,
                client;
  l4_umword_t *esp=(l4_umword_t*)stack;
  
  l4_thread_ex_regs(l4_myself(), -1, -1, &preempter, &pager,
                    &dummy, &dummy, &dummy);
  client = l4_myself();
  client.id.lthread = thread;
  
  *--esp = irq;
  *--esp = port;
  *--esp = 0;
  l4_thread_ex_regs(client, (l4_umword_t)function, (l4_umword_t)esp, 
                    &preempter, &pager,
                    &dummy, &dummy, &dummy);
}

int main(int argc, char*argv[]){
  rmgr_init();
  LOG_init("pit");
  
  
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
