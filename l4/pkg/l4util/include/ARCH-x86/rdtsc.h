/**
 * \file   l4util/include/ARCH-x86/rdtsc.h
 * \brief  time stamp counter related functions
 *
 * \date   Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __l4_rdtsc_h
#define __l4_rdtsc_h

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>

/* interface */

/** \defgroup rdtsc Time Stamp Counter Related Functions */

extern l4_uint32_t l4_scaler_tsc_to_ns;
extern l4_uint32_t l4_scaler_tsc_to_us;
extern l4_uint32_t l4_scaler_ns_to_tsc;
extern l4_uint32_t l4_scaler_tsc_linux;

/** Return current value of CPU-internal time stamp counter
 * \ingroup rdtsc
 * \return 64-bit time stamp */
L4_INLINE l4_cpu_time_t
l4_rdtsc (void);

/** Convert time stamp counter into ns
 * \ingroup rdtsc
 * \param tsc time value in CPU ticks
 * \return time value in ns
 */
L4_INLINE l4_uint64_t
l4_tsc_to_ns (l4_cpu_time_t tsc);

/** Convert time stamp counter into micro seconds
 * \ingroup rdtsc
 * \param tsc time value in CPU ticks
 * \return time value in micro seconds
 */
L4_INLINE l4_uint64_t
l4_tsc_to_us (l4_cpu_time_t tsc);

/** Convert time stamp counter into s.ns
 * \ingroup rdtsc
 * \param tsc time value in CPU ticks
 * \retval s seconds
 * \retval ns nano seconds
 */
L4_INLINE void
l4_tsc_to_s_and_ns (l4_cpu_time_t tsc, l4_uint32_t *s, l4_uint32_t *ns);

/** Convert nano seconds into CPU ticks
 * \ingroup rdtsc
 * \param ns nano seconds
 * \return CPU ticks
 */
L4_INLINE l4_cpu_time_t
l4_ns_to_tsc (l4_uint64_t ns);

/** Wait busy for a small amount of time
 * \ingroup rdtsc
 * \param ns nano seconds to wait
 */
L4_INLINE void
l4_busy_wait_ns (l4_uint64_t ns);

/** Wait busy for a small amount of time
 * \ingroup rdtsc
 * \param us micro seconds to wait
 */
L4_INLINE void
l4_busy_wait_us (l4_uint64_t us);

EXTERN_C_BEGIN

/** Determine some scalers to be able to convert between real time and CPU
 * ticks. This test uses channel 0 of the PIT (8254)
 * \ingroup rdtsc
 */
l4_uint32_t
l4_calibrate_tsc (void);

/** Get CPU frequency in Hz
 * \ingroup rdtsc
 * \return frequency in Hz
 */
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

