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

#define L4_TSC_INIT_AUTO             0
#define L4_TSC_INIT_KERNEL           1
#define L4_TSC_INIT_CALIBRATE        2

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

/* the same, but only 32 bit. Useful for smaller differences, 
   needs less cycles. */
L4_INLINE
l4_uint32_t l4_rdtsc_32(void);

/** Return current value of CPU-internal performance measurement counter
 * \ingroup rdtsc
 * \param  nr		Number of counter (0 or 1)
 * \return 64-bit PMC */
L4_INLINE l4_cpu_time_t
l4_rdpmc (int nr);

/* the same, but only 32 bit. Useful for smaller differences, 
   needs less cycles. */
L4_INLINE
l4_uint32_t l4_rdpmc_32(int nr);


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
 * ticks. This test uses channel 0 of the PIT (i8254).
 * Just calls l4_tsc_init(L4_TSC_INIT_AUTO).
 * \ingroup rdtsc
 */
L4_INLINE l4_uint32_t
l4_calibrate_tsc (void);

/** Initialize the scalers needed by l4_tsc_to_ns()/l4_ns_to_tsc() and so on.
 * Current versions of Fiasco export these scalers from kernel into userland.
 * The programmer may decide whether he allows to use these scalers or if an
 * calibration should be performed.
 * \param   constraint   programmers constraint:
 *                       - #L4_TSC_INIT_AUTO if the kernel exports the scalers
 *                         then use them. If not, perform calibration using
 *                         channel 0 of the PIT (i8254). The latter case may
 *                         lead into short (unpredictable) periods where
 *                         interrupts are disabled.
 *                       - #L4_TSC_INIT_KERNEL depend on retrieving the scalers
 *                         from kernel. If the scalers are not available,
 *                         return 0.
 *                       - #L4_TSC_INIT_CALIBRATE Ignore possible scalers
 *                         exported by the scaler, instead insist on
 *                         calibration using the PIT.
 * \return 0 on error (no scalers exported by kernel, calibrating failed ...)
 *         otherwise returns (2^32 / (tsc per µsec)). This value has the
 *         same semantics as the value returned by the calibrate_delay_loop()
 *         function of the Linux kernel. 
 * \ingroup rdtsc 
 */
l4_uint32_t
l4_tsc_init (int constraint);

/** Get CPU frequency in Hz
 * \ingroup rdtsc
 * \return frequency in Hz
 */
l4_uint32_t
l4_get_hz (void);

EXTERN_C_END

/* implementaion */

L4_INLINE l4_uint32_t
l4_calibrate_tsc (void)
{
  return l4_tsc_init(L4_TSC_INIT_AUTO);
}

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
L4_INLINE
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

L4_INLINE l4_cpu_time_t
l4_rdpmc (int nr)
{
    l4_cpu_time_t v;
    
    __asm__ __volatile__ (
	 "rdpmc				\n\t"
	:
	"=A" (v)
	: "c" (nr)
	);
    
    return v;
}

/* the same, but only 32 bit. Useful for smaller differences, 
   needs less cycles. */
L4_INLINE
l4_uint32_t l4_rdpmc_32(int nr)
{
  l4_uint32_t x;

  __asm__ __volatile__ (
       "rdpmc				\n\t"
       : "=a" (x)
       : "c" (nr)
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
    l4_uint64_t us;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	:"=A" (us),
	 "=&c" (dummy)
	:"0" (tsc),
	 "g" (l4_scaler_tsc_to_us)
	);
    return us;
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

