#include <l4/util/irq.h>
#include <l4/log/l4log.h>
#include "config.h"
#include "pic.h"

#ifdef USE_LOCKING
#include "lock.h"
static wq_lock_queue_base pic_wq = {NULL};
#endif

#ifdef USE_CLISTI
unsigned char static volatile inline l4_cli (void){
  int r;
  __asm__ ("cli"
           );
  return r;
}

unsigned char static volatile inline l4_sti (void){
  int r;
  __asm__ ("sti"
           );
  return r;
}
#else
#define l4_cli()
#define l4_sti()
#endif

void irq_mask(int irq){
#ifdef USE_LOCKING
  wq_lock_queue_elem wqe;

  if(wq_lock_lock(&pic_wq, &wqe)) LOGl("error locking pic-waitqueue");
#endif

  l4_cli();
  if(irq<8){
    __l4_outb(0x21, __l4_inb(0x21) | (1<<irq));  
  } else {
    __l4_outb(0xa1, __l4_inb(0xa1) | (1<<(irq-8)));  
  }
  l4_sti();
  
#ifdef USE_LOCKING
  if(wq_lock_unlock(&pic_wq, &wqe)) LOGl("error unlocking pic-waitqueue");
#endif
}

void irq_unmask(int irq){
#ifdef USE_LOCKING
  wq_lock_queue_elem wqe;

  wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if(irq<8){
    __l4_outb(0x21, __l4_inb(0x21) & ~(1<<irq));  
  } else {
    __l4_outb(0xa1, __l4_inb(0xa1) & ~(1<<(irq-8)));
  }
  l4_sti();
  
#ifdef USE_LOCKING
  wq_lock_unlock(&pic_wq, &wqe);
#endif
}

void irq_ack(int irq){
#ifdef USE_LOCKING
  wq_lock_queue_elem wqe;

  wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
    if (irq > 7){
      __l4_outb(0xA0,0x60|(irq&7));
      __l4_outb(0xA0,0x0B);
      if (__l4_inb(0xA0) == 0)  __l4_outb(0x20, 0x62);
    }else{
      __l4_outb(0x20,0x60|irq);
    }
  l4_sti();
  
#ifdef USE_LOCKING
  wq_lock_unlock(&pic_wq, &wqe);
#endif
}

/* return the interrupt service register (isr). This register holds the
   irq's which are accepted by the processor and not acknowledged.  if
   master!=0, return master isr, else return slave isr */
int pic_isr(int master){
#ifdef USE_LOCKING
  wq_lock_queue_elem wqe;
#endif
  int dat;

#ifdef USE_LOCKING
  wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if (master){
    __l4_outb(0xA0,0xb);
    dat = __l4_inb(0xa0);
  }else{
    __l4_outb(0x20,0xb);
    dat = __l4_inb(0x20);
  }
  l4_sti();

#ifdef USE_LOCKING
  wq_lock_unlock(&pic_wq, &wqe);
#endif
  
  return dat;
}

/* return the interrupt request register (irr). This register holds the
   irq's which are requested by hardware but not delivered to the processor. 
   if master!=0, return master irr, else return slave irr */
int pic_irr(int master){
#ifdef USE_LOCKING
  wq_lock_queue_elem wqe;
#endif
  int dat;

#ifdef USE_LOCKING
  wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if (master){
    __l4_outb(0xA0,0xa);
    dat = __l4_inb(0xa0);
  }else{
    __l4_outb(0x20,0xa);
    dat = __l4_inb(0x20);
  }
  l4_sti();

#ifdef USE_LOCKING
  wq_lock_unlock(&pic_wq, &wqe);
#endif
  
  return dat;
}

/* return the interrupt mask register.
   if master!=0, return master isr, else return slave isr */
int pic_imr(int master){
#ifdef USE_LOCKING
  wq_lock_queue_elem wqe;
#endif
  int dat;

#ifdef USE_LOCKING
  wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if (master){
    dat = __l4_inb(0xa1);
  }else{
    dat = __l4_inb(0x21);
  }
  l4_sti();

#ifdef USE_LOCKING
  wq_lock_unlock(&pic_wq, &wqe);
#endif
  
  return dat;
}
