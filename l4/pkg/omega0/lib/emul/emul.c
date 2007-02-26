/*!
 * \file   omega0/lib/emul/emul.c
 * \brief  Thumb emulation of some omega0-functions.
 *
 * \date   02/19/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/util/irq.h>
#include <l4/omega0/client.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/ipc.h>

static void irq_mask(int irq);
static void irq_unmask(int irq);
static void irq_ack(int irq);

static unsigned int masked=0;

/*\brief Attach to an interrupt
 *
 * Limitation: You can attach to exactly one(1) interrupt.
 *
 * \retval -1	the lib was used to attach to an interrupt, and you
 *		try to attach to another one.
 * \retval -2	could not get interrupt from rmgr.
 * \retval -3	could not attach to the interrupt at kernel level.
 */
int omega0_attach(omega0_irqdesc_t desc){
  l4_threadid_t irq_th;
  l4_umword_t dummy;
  l4_msgdope_t result;
  int error, handle, irq=desc.s.num-1;
  static int old_irq = -1;

  /* What exactly is this good for? */
  if(old_irq!=-1){
    if(old_irq!=irq) return -1;
    handle = irq+1;
    return 0;
  }

  if(rmgr_get_irq(irq)) return -2;
  irq_th.lh.low = irq + 1;
  irq_th.lh.high = 0;
  
  error = l4_i386_ipc_receive(irq_th, 0, &dummy, &dummy,
                              L4_IPC_TIMEOUT(0,1,0,1,0,0), &result);

  if(error!=L4_IPC_RETIMEOUT) return -3;
  handle = irq+1;
  old_irq = irq;
  irq_mask(irq);
  return handle;
}

int omega0_detach(omega0_irqdesc_t desc){
  return 0;	// dont detach
}

int omega0_request(int handle, omega0_request_t request){
  l4_threadid_t irq_th;
  int err, irq = request.s.param-1;
  l4_umword_t dummy;
  l4_msgdope_t result;
  
  if(request.s.consume){
    irq_unmask(irq);
  }
  if(request.s.unmask){
    irq_unmask(irq);
  }
  if(request.s.wait){
    /* We implement the auto-consume feature */
    if(masked & (1<<(irq))){
      irq_unmask(irq);
    }
    irq_th.lh.low = handle;
    irq_th.lh.high = 0;
    err = l4_i386_ipc_receive(irq_th, L4_IPC_SHORT_MSG, &dummy, &dummy,
                              L4_IPC_NEVER, &result);
    irq_mask(handle-1);
    irq_ack(handle-1);
    if(err) return err;
  }
  return 0;
}


/***************************************************************************
 * PIC functions
 **************************************************************************/
unsigned char extern inline l4_cli (void);
unsigned char extern inline l4_cli (void){
  int r;
  __asm__ ("cli"
           );
  return r;
}

unsigned char extern inline l4_sti (void);
unsigned char extern inline l4_sti (void){
  int r;
  __asm__ ("sti"
           );
  return r;
}

static void irq_mask(int irq){
  l4_cli();
  if(irq<8){
    __l4_outb(0x21, __l4_inb(0x21) | (1<<irq));  
  } else {
    __l4_outb(0xa1, __l4_inb(0xa1) | (1<<(irq-8)));  
  }
  masked |= 1<<irq;
  l4_sti();
}

void irq_unmask(int irq){
  l4_cli();
  if(irq<8){
    __l4_outb(0x21, __l4_inb(0x21) & ~(1<<irq));  
  } else {
    __l4_outb(0xa1, __l4_inb(0xa1) & ~(1<<(irq-8)));
  }
  masked &= ~(1<<irq);
  l4_sti();
}

static void irq_ack(int irq){
  l4_cli();
    if (irq > 7){
      __l4_outb(0xA0,0x60|(irq&7));
      __l4_outb(0xA0,0x0B);
      if (__l4_inb(0xA0) == 0)  __l4_outb(0x20, 0x62);
    }else{
      __l4_outb(0x20,0x60|irq);
    }
  l4_sti();
}
