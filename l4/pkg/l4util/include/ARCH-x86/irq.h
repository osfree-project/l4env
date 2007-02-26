/* 
 * $Id$
 */
#ifndef __L4_IRQ_H__ 
#define __L4_IRQ_H__ 

#include <l4/sys/compiler.h>

L4_INLINE void irq_acknowledge(unsigned int irq);

void static volatile inline __l4_outb(int p, unsigned char r)
{
  __asm__ __volatile__ ("outb %b0, %w1"
           :
           : "a" ((unsigned char)r), "Nd"((unsigned short)p)
           );
}

unsigned char static volatile inline __l4_inb (int p)
{
  unsigned char r;
  
  __asm__ __volatile__ ("inb %w1, %b0"
           : "=a" (r)
           :  "Nd" ((unsigned short)p)
           );
  return r;
}

unsigned char static volatile inline __l4_cli (void)
{
  int r;
  __asm__ ("cli"
           );
  return r;
}

unsigned char static volatile inline __l4_sti (void)
{
  int r;
  __asm__ ("sti"
           );
  return r;
}

void static volatile inline __l4_flags_save (unsigned *flags)
{
  __asm__ __volatile__ ("pushfl ; popl %0 " :"=g" (*flags) : :"memory");
}

void static volatile inline __l4_flags_restore (unsigned *flags)
{
  __asm__ __volatile__ ("pushl %0 ; popfl" : :"g" (*flags) : "memory");
}

/* Acknowledge the given irq. Special fully nested mode of the pic is assumed
   as in L4. 
*/
L4_INLINE void irq_acknowledge(unsigned int irq)
{
  if (irq > 7)
    {
      __l4_outb(0xA0,0x60+(irq & 7));
      __l4_outb(0xA0,0x0B);
      if (__l4_inb(0xA0) == 0)
	__l4_outb(0x20, 0x60 + 2);
    } 
  else
    {
      __l4_outb(0x20, 0x60+irq);     /* acknowledge the irq */
    }
};

#endif
