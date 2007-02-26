#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/omega0/client.h>
#include <l4/sys/kdebug.h>

char LOG_tag[9]="serial";

int port1 = 0x3f8;	// com1
int irq1  = 4;		// irq 4
int port2 = 0x2f8;	// com2
int irq2  = 3;		// irq 3
int shared = 1;		// shared irq or not

void list(void);
extern void serial_init(int, int);
extern void outb(int port, unsigned char val);
extern unsigned char inb(int port);
int attach(int, int*);

void list(void){
  int irq;
  
  for(irq=omega0_first(); irq>0; irq=omega0_next(irq)){
    LOG("irq %x available", irq);
  }
}

int attach(int irq, int*handle){
  omega0_irqdesc_t desc;

  if(!handle){
    LOGl("error: given handle points to %p", handle);
    return 1;
  }
  
  desc.s.shared = shared;
  desc.s.num = irq+1;
  
  if((*handle=omega0_attach(desc))<0){
    LOGl("error %#x attaching to irq %x", *handle, irq);
    return 1;
  }
  return 0;
}

void serial_stuff(int port, int irq, int handle);
void serial_stuff(int port, int irq, int handle){
  omega0_request_t request;
  int err, i;
  unsigned char stat, dat, *to_send="", send_open=0;
  char prompt[]="hi> ";

  request.s.unmask = 1;
  request.s.consume = 0;
  for(i=0;i<sizeof(prompt);i++){
    outb(port, prompt[i]);
    l4_sleep(100);
  }
  outb(port, '\r');l4_sleep(100);
  outb(port, '\n');
  while(1){
    request.s.param = irq+1;
    request.s.wait = 1;
    request.s.mask = 0;
    err = omega0_request(handle, request);
    if(err<0) LOGl("omega0_request(handle=%d, request=%#x) returned %d",
                   handle, request.i, err);
                   
    do{
      stat = inb(port+2);
      if(stat & (1<<2)) {
        dat = inb(port);
        LOG("port %#x: rcvd char '%c' (%#x)", port, dat>=32?dat:'.', dat);
        if(dat==13) {
          to_send="\n\r";
          send_open=*to_send;
          outb(port, *to_send++);
        } else if(dat=='d'){
          enter_kdebug("d pressed");
        } else {
          outb(port, dat);
          send_open=dat;
        }
      } else if(stat & (1<<1)){
        LOG("port %#x: char sent\n", port);
        if(*to_send) {
          send_open=*to_send;
          outb(port, *to_send++);
        } else {
          send_open=0;
        }
      } else {
        LOG("port %#x: stat %#x", port, stat);
      }
    } while(stat!=1);
    request.s.unmask = 0;
    request.s.consume = 1;
  }
}

void thread_fn(int port, int irq);
void thread_fn(int port, int irq){
  int handle;

  LOG("initalising serial line at %#x, int %x", port, irq);
  serial_init(port, irq);
  
  LOG("attaching to irq %x", irq);
  if(attach(irq, &handle))while(1){
    l4_sleep(-1);
  }
  
  LOG("playing with serial line at port %#x", port);
  serial_stuff(port, irq, handle);

  LOGL("returned, idling now");
  while(1) l4_sleep(100);

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

#define STACKSIZE 1024
#define MAXTHREADS 2
unsigned char stacks[MAXTHREADS][STACKSIZE];

int main(int argc, char*argv[]){
  LOG("listing of available irqs...\n");
  list();

  start_thread(1, thread_fn, &stacks[0][STACKSIZE], port1, irq1);
  start_thread(2, thread_fn, &stacks[1][STACKSIZE], port2, irq2);

  LOG("falling asleep");
  while(1)l4_sleep(1000);

  return 0;
}
