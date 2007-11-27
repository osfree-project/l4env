/*!
 * \file   rtc/lib/libc_backend/time/gettime.h
 * \brief  architecture specific handling
 *
 * \date   2007-11-23
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RTC__LIB__LIBC_BACKEND__TIME__GETTIME_H__
#define __RTC__LIB__LIBC_BACKEND__TIME__GETTIME_H__

#if defined(ARCH_x86) || defined(ARCH_amd64)
#include <l4/util/rdtsc.h>
#include <l4/sys/l4int.h>

static inline void libc_backend_rtc_init(void)
{
  l4_calibrate_tsc();
}

static inline void libc_backend_rtc_get_s_and_ns(l4_uint32_t *s, l4_uint32_t *ns)
{
  l4_tsc_to_s_and_ns(l4_rdtsc(), s, ns);
}

#elif defined(ARCH_arm)

#include <l4/sys/l4int.h>

static inline void libc_backend_rtc_init(void)
{
}

static inline void libc_backend_rtc_get_s_and_ns(l4_uint32_t *s, l4_uint32_t *ns)
{
  *s = *ns = 0;
}

#else
#error Unknown architecture
#endif

#endif /* ! __RTC__LIB__LIBC_BACKEND__TIME__GETTIME_H__ */
