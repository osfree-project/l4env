/* 
 * $Id$
 */

#ifndef __UTIL_H
#define __UTIL_H

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/** Calculate l4 timeouts */
int micros2l4to(int mus, int *to_e, int *to_m);

/** Suspend thread for a period of <ms> milliseconds */
void l4_sleep(int ms);

/* Suspend thread for a period of <us> micro seconds.
 * WARNING: This function is mostly bogus since the timer resolution of
 *          current L4 implementations is about 1ms! */
void l4_usleep(int us);

/** Go sleep and never wake up. */
L4_INLINE void l4_sleep_forever(void) __attribute__((noreturn));

L4_INLINE void
l4_sleep_forever(void)
{
  for (;;)
    {
      __asm__ __volatile("xor  %%eax,%%eax  \n\t"
                         "mov  %%eax,%%ebp  \n\t"
		     	 "mov  %%eax,%%esi  \n\t"
			 "mov  %%eax,%%edi  \n\t"
			 "mov  %%eax,%%ecx  \n\t"
			 "dec  %%eax        \n\t"
			 "int  $0x30        \n\t"
			 : : : "memory");
    }
}

/** Touch data areas to force mapping read-only */
static inline void
l4_touch_ro(const void*addr, unsigned size)
{
  const char *bptr, *eptr;

  bptr = (const char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE){
    asm volatile("or	%0,%%eax \n"
                 :
                 : "m" (*(const unsigned*)bptr)
                 : "eax" );
  }
}


/** Touch data areas to force mapping read-write */
static inline void
l4_touch_rw(const void*addr, unsigned size)
{
  const char *bptr, *eptr;
      
  bptr = (const char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE){
    asm volatile("or	$0,%0 \n"
                 :
                 : "m" (*(const unsigned*)bptr)
                 );
  }
}

EXTERN_C_END

#endif

