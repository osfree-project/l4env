#include <l4/sys/compiler.h>
#include "serial.h"

static inline void outb(int p, unsigned int r){
  __asm__ ("outb %%al, %%dx"
           :
           : "a"(r),
             "d"(p)
           );
}

static inline unsigned char inb (int p){
  int r;
  __asm__ ("inb %%dx, %%al"
           : "=a" (r)
           :  "d"(p)
           );
  return r;
}


/* init the serial line belonging to port with 115000 baud, 8n1,
   irq's on incoming and delivery. No pic programming will take place. */
void serial_init(int port, int irq){
  // select bitrate with 115.200 bps
  outb(port+3, inb(port+3) | 0x80);	// set dlab
  outb(port, 1);
  outb(port+1, 0);
  outb(port+3, 0x3);	// reset dlab, reset break, no parity, 1 stop, 8 data

  // enable irq on xmit and rcv
  outb(port+1, 3);

  // enable master-irq-bit, set dtr and rts
  outb(port+4, inb(port+4)|0x08|0x01|0x02);
}

void serial_send_char(int port, char snd){
  outb(port, snd);
}
