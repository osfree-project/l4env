/* 
 * $Id$
 */

#ifndef __UTIL_H
#define __UTIL_H

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/** Calculate l4 timeouts
 * \param mus	time in microseconds. Special cases:
 *		- 0 - > timeout 0
 *		- -1 -> timeout NEVER
 * \param to_m	contains l4-mantissa after return
 * \param to_e	contains l4-exponent after return
 */
l4_timeout_s l4util_micros2l4to(int mus);

/** Calculate l4 timeouts
 * \param to_m	contains l4-mantissa
 * \param to_e	contains l4-exponent
 * \returns	time in microseconds. Special cases:
 *		- 0 - > timeout 0
 *		- -1 -> timeout NEVER or invalid params
 */
int l4util_l4to2micros(int to_m, int to_e);

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
      __asm__ __volatile("xor  %%rax,%%rax  \n\t"
                         "mov  %%rax,%%rbp  \n\t"
		     	 "mov  %%rax,%%rsi  \n\t"
			 "mov  %%rax,%%rdi  \n\t"
			 "mov  %%rax,%%rcx  \n\t"
			 "dec  %%rax        \n\t"
			 "int  $0x30        \n\t"
			 : : : "memory");
    }
}

/** Touch data areas to force mapping read-only */
static inline void
l4_touch_ro(const void*addr, unsigned size)
{
  const char *bptr, *eptr;

  bptr = (const char*)(((l4_addr_t)addr) & L4_PAGEMASK);
  eptr = (const char*)(((l4_addr_t)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE){
    asm volatile("or	%0,%%rax \n"
                 :
                 : "m" (*(const unsigned*)bptr)
                 : "rax" );
  }
}


/** Touch data areas to force mapping read-write */
static inline void
l4_touch_rw(const void*addr, unsigned size)
{
  const char *bptr, *eptr;
      
  bptr = (const char*)(((l4_addr_t)addr) & L4_PAGEMASK);
  eptr = (const char*)(((l4_addr_t)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE){
    asm volatile("or	$0,%0 \n"
                 :
                 : "m" (*(const unsigned*)bptr)
                 );
  }
}

EXTERN_C_END

#endif

