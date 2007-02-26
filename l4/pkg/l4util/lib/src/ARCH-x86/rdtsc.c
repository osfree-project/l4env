#include <l4/util/rdtsc.h>

l4_uint32_t l4_scaler_tsc_to_ns = 0;
l4_uint32_t l4_scaler_tsc_to_us = 0;
l4_uint32_t l4_scaler_ns_to_tsc = 0;

static inline unsigned char
inb (unsigned short port)
{
  unsigned char _v;
  asm volatile ("inb   %w1, %b0\n\t"
               :"=a" (_v)
               :"Nd" (port));
  return _v;
}

static inline void
outb (unsigned char value, unsigned short port) 
{
  asm volatile ("outb  %b0, %w1\n\t"
               : 
               :"a" (value), 
                "Nd" (port));
}

static inline l4_uint32_t
muldiv (l4_uint32_t a, l4_uint32_t mul, l4_uint32_t div)
{
  asm ("mull %1 ; divl %2\n\t"
       :"=a" (a)
       :"d" (mul), "c" (div), "0" (a));
  return a;
}



/*
 * Return 2^32 / (tsc clocks per usec) 
 */

l4_uint32_t
l4_calibrate_tsc (void)
{
#define CLOCK_TICK_RATE 	1193180
#define CALIBRATE_TIME  	50001
#define CALIBRATE_LATCH		(CLOCK_TICK_RATE / 20) /* 50 ms */

  /* Set the Gate high, disable speaker */
  outb ((inb (0x61) & ~0x02) | 0x01, 0x61);

  outb (0xb0, 0x43);		/* binary, mode 0, LSB/MSB, Ch 2 */
  outb (CALIBRATE_LATCH & 0xff, 0x42);	/* LSB of count */
  outb (CALIBRATE_LATCH >> 8, 0x42);	/* MSB of count */

  {
    l4_uint64_t tsc_start, tsc_end;
    l4_uint32_t count;
    l4_uint32_t flags;
    l4_uint32_t tsc_to_ns_div;
    l4_uint32_t dummy;

    /* disable interrupts */
    asm volatile ("pushfl ; popl %0 ; cli " :"=g" (flags) : :"memory");
    
    tsc_start = l4_rdtsc ();
    count = 0;
    do
      {
	count++;
      }
    while ((inb (0x61) & 0x20) == 0);
    tsc_end = l4_rdtsc ();
    
    /* restore flags */
    asm volatile ("pushl %0 ; popfl" : :"g" (flags) : "memory");

    /* Error: ECTCNEVERSET */
    if (count <= 1)
      goto bad_ctc;

    /* 64-bit subtract - gcc just messes up with long longs */
    tsc_end -= tsc_start;

    /* Error: ECPUTOOFAST */
    if (tsc_end & 0xffffffff00000000LL)
      goto bad_ctc;

    /* Error: ECPUTOOSLOW */
    if ((tsc_end & 0xffffffffL) <= CALIBRATE_TIME)
      goto bad_ctc;
    
    asm ("divl %2"
	:"=a" (tsc_to_ns_div), "=d" (dummy)
	:"r" ((l4_uint32_t)tsc_end),  "0" (0), "1" (CALIBRATE_TIME));

    l4_scaler_tsc_to_ns = muldiv(tsc_to_ns_div, 1000, 1<<5);
    l4_scaler_tsc_to_us = muldiv(tsc_to_ns_div,    1, 1<<5);
    l4_scaler_ns_to_tsc = muldiv(((1ULL<<32)/1000ULL), (l4_uint32_t)tsc_end,
	                         CALIBRATE_TIME * (1<<5));

    /* l4_scaler_tsc_to_ns = (2^32 * 1.000.000.000) / (32 * Hz)
     * l4_scaler_ns_to_tsc = (2^32 * Hz) / (32 * 1.000.000.000)
     * l4_scaler_tsc_to_us = (2^32 * 1.000.000)     / (32 * Hz) */

    return tsc_to_ns_div;
  }

  /*
   * The CTC wasn't reliable: we got a hit on the very first read,
   * or the CPU was so fast/slow that the quotient wouldn't fit in
   * 32 bits..
   */
bad_ctc:
  return 0;
}

l4_uint32_t
l4_get_hz (void)
{
  if (!l4_scaler_tsc_to_ns)
    return 0;

  return (l4_uint32_t)(l4_ns_to_tsc(1000000000UL));
}

