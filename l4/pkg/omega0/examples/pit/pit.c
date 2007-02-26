#include <l4/sys/compiler.h>

void pit_set_freq(int);

void outb(int p, unsigned int r);
void outb(int p, unsigned int r){
  __asm__ ("outb %%al, %%dx"
           :
           : "a"(r),
             "d"(p)
           );
}

unsigned char inb (int p);
unsigned char inb (int p){
  int r;
  __asm__ ("inb %%dx, %%al"
           : "=a" (r)
           :  "d"(p)
           );
  return r;
}


void pit_set_freq(int freq){
  // set frequency for timer 0 at 8284-PIT
  int val;
  
  if(!freq)freq=1;
  val = 1193180/freq;
  if(val>=65536) val = 0;
  
  outb(0x43, 0x34);	// write lower and higher byte of counter 0,
  			// mode 2 (periodic impulses), no bcd
  outb(0x40, val&0xff);
  outb(0x40, val>>8);
}
