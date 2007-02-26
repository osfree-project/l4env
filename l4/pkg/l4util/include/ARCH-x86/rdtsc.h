#ifndef __l4_rdtsc_h
#define __l4_rdtsc_h

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>

/* interface */

extern l4_uint32_t l4_scaler_tsc_to_ns;
extern l4_uint32_t l4_scaler_tsc_to_us;
extern l4_uint32_t l4_scaler_ns_to_tsc;

L4_INLINE l4_cpu_time_t
l4_rdtsc (void);

L4_INLINE l4_uint64_t
l4_tsc_to_ns (l4_cpu_time_t tsc);

L4_INLINE l4_uint64_t
l4_tsc_to_us (l4_cpu_time_t tsc);

L4_INLINE void
l4_tsc_to_s_and_ns (l4_cpu_time_t tsc, l4_uint32_t *s, l4_uint32_t *ns);

L4_INLINE l4_cpu_time_t
l4_ns_to_tsc (l4_uint64_t ns);

L4_INLINE void
l4_busy_wait_ns (l4_uint64_t ns);

L4_INLINE void
l4_busy_wait_us (l4_uint64_t us);

EXTERN_C_BEGIN

l4_uint32_t
l4_calibrate_tsc (void);

l4_uint32_t
l4_get_hz (void);

EXTERN_C_END

/* implementaion */

L4_INLINE l4_cpu_time_t
l4_rdtsc (void)
{
    l4_cpu_time_t v;
    
    __asm__ __volatile__ 
	("				\n\t"
	 ".byte 0x0f, 0x31		\n\t"
	/*"rdtsc\n\t"*/
	:
	"=A" (v)
	: /* no inputs */
	);
    
    return v;
}

/* the same, but only 32 bit. Useful for smaller differences, 
   needs less cycles. */
static inline 
l4_uint32_t l4_rdtsc_32(void)
{
  l4_uint32_t x;

  __asm__ __volatile__ (
       ".byte 0x0f, 0x31\n\t"	// rdtsc
       : "=a" (x)
       :
       : "edx");
    
  return x;
}

L4_INLINE l4_uint64_t
l4_tsc_to_ns (l4_cpu_time_t tsc)
{
    l4_uint32_t dummy;
    l4_uint64_t ns;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (ns),
	 "=&c" (dummy)
	:"0" (tsc),
	 "g" (l4_scaler_tsc_to_ns)
	);
    return ns;
}

L4_INLINE l4_uint64_t
l4_tsc_to_us (l4_cpu_time_t tsc)
{
    l4_uint32_t dummy;
    l4_uint64_t ns;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (ns),
	 "=&c" (dummy)
	:"0" (tsc),
	 "g" (l4_scaler_tsc_to_us)
	);
    return ns;
}

L4_INLINE void
l4_tsc_to_s_and_ns (l4_cpu_time_t tsc, l4_uint32_t *s, l4_uint32_t *ns)
{
    l4_uint32_t dummy;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%4			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%4			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "movl  $1000000000, %%ecx	\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	 "divl  %%ecx			\n\t"
	:"=a" (*s), "=d" (*ns), "=c" (dummy)
	: "A" (tsc), "g" (l4_scaler_tsc_to_ns)
	);
}

L4_INLINE l4_cpu_time_t
l4_ns_to_tsc (l4_uint64_t ns)
{
    l4_uint32_t dummy;
    l4_cpu_time_t tsc;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (tsc),
	 "=&c" (dummy)
	:"0" (ns),
	 "g" (l4_scaler_ns_to_tsc)
	);
    return tsc;
}

L4_INLINE void
l4_busy_wait_ns (l4_uint64_t ns)
{
  l4_cpu_time_t stop = l4_rdtsc();
  stop += l4_ns_to_tsc(ns);
  
  while (l4_rdtsc() < stop)
    ;
}

L4_INLINE void
l4_busy_wait_us (l4_uint64_t us)
{
  l4_cpu_time_t stop = l4_rdtsc ();
  stop += l4_ns_to_tsc(us*1000ULL);
  
  while (l4_rdtsc() < stop)
    ;
}

#endif /* __l4_rdtsc_h */

