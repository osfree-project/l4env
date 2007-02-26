#include <l4/util/irq.h>
#include <l4/log/l4log.h>
#include "config.h"
#include "pic.h"

#ifdef USE_LOCKING
#include <l4/util/lock_wq.h>
static l4util_wq_lock_queue_base_t pic_wq = {NULL};
#endif

#ifdef USE_CLISTI
#define l4_cli l4util_cli
#define l4_sti l4util_sti
#else
#define l4_cli()
#define l4_sti()
#endif

void irq_mask(int irq){
#ifdef USE_LOCKING
  l4util_wq_lock_queue_elem_t wqe;

  if(l4util_wq_lock_lock(&pic_wq, &wqe)) LOGl("error locking pic-waitqueue");
#endif

  l4_cli();
  if(irq<8){
    l4util_out8(l4util_in8(0x21) | (1<<irq), 0x21);  
  } else {
    l4util_out8(l4util_in8(0xa1) | (1<<(irq-8)), 0xa1);  
  }
  l4_sti();
  
#ifdef USE_LOCKING
  if(l4util_wq_lock_unlock(&pic_wq, &wqe))
      LOGl("error unlocking pic-waitqueue");
#endif
}

void irq_unmask(int irq){
#ifdef USE_LOCKING
  l4util_wq_lock_queue_elem_t wqe;

  l4util_wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if(irq<8){
    l4util_out8(l4util_in8(0x21) & ~(1<<irq), 0x21);  
  } else {
    l4util_out8(l4util_in8(0xa1) & ~(1<<(irq-8)), 0xa1);
  }
  l4_sti();
  
#ifdef USE_LOCKING
  l4util_wq_lock_unlock(&pic_wq, &wqe);
#endif
}

void irq_ack(int irq){
#ifdef USE_LOCKING
  l4util_wq_lock_queue_elem_t wqe;
  l4util_wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  l4util_irq_acknowledge(irq);
  l4_sti();
  
#ifdef USE_LOCKING
  l4util_wq_lock_unlock(&pic_wq, &wqe);
#endif
}

/* return the interrupt service register (isr). This register holds the
   irq's which are accepted by the processor and not acknowledged.  if
   master!=0, return master isr, else return slave isr */
int pic_isr(int master){
  int dat;
#ifdef USE_LOCKING
  l4util_wq_lock_queue_elem_t wqe;
  l4util_wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if (master){
    l4util_out8(0xb,0xA0);
    dat = l4util_in8(0xa0);
  }else{
    l4util_out8(0xb,0x20);
    dat = l4util_in8(0x20);
  }
  l4_sti();

#ifdef USE_LOCKING
  l4util_wq_lock_unlock(&pic_wq, &wqe);
#endif
  
  return dat;
}

/* return the interrupt request register (irr). This register holds the
   irq's which are requested by hardware but not delivered to the processor. 
   if master!=0, return master irr, else return slave irr */
int pic_irr(int master){
  int dat;
#ifdef USE_LOCKING
  l4util_wq_lock_queue_elem_t wqe;
  l4util_wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if (master){
    l4util_out8(0xa,0xA0);
    dat = l4util_in8(0xa0);
  }else{
    l4util_out8(0xa,0x20);
    dat = l4util_in8(0x20);
  }
  l4_sti();

#ifdef USE_LOCKING
  l4util_wq_lock_unlock(&pic_wq, &wqe);
#endif
  
  return dat;
}

/* return the interrupt mask register.
   if master!=0, return master isr, else return slave isr */
int pic_imr(int master){
  int dat;
#ifdef USE_LOCKING
  l4util_wq_lock_queue_elem_t wqe;
  l4util_wq_lock_lock(&pic_wq, &wqe);
#endif
  
  l4_cli();
  if (master){
    dat = l4util_in8(0xa1);
  }else{
    dat = l4util_in8(0x21);
  }
  l4_sti();

#ifdef USE_LOCKING
  l4util_wq_lock_unlock(&pic_wq, &wqe);
#endif
  
  return dat;
}
