/**
 * \file   rtc/lib/libc_backends/time/gettimeofday.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <sys/time.h>

#include <l4/util/rdtsc.h>
#include <l4/sys/l4int.h>

extern unsigned int offset;

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    l4_uint32_t s, ns;

    // fixme: get KIP time or rdtsc or other time source
    l4_tsc_to_s_and_ns(l4_rdtsc(), &s, &ns);
    // add both and copy sum to struct
    if (tv)
    {
        tv->tv_sec = s + offset;
        tv->tv_usec = ns / 1000;
    }

    if (tz)
    {
        // man 2 gettimeofday says the use of the 'tz' argument is
        // deprecated, but we should still fill in something here to
        // avoid bogus values
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }

    return 0;
}
