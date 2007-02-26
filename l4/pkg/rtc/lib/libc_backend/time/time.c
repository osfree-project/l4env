/**
 * \file   dietlibc/lib/backends/time/time.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <time.h>

#include <l4/util/rdtsc.h>
#include <l4/sys/l4int.h>

extern unsigned int offset;

time_t time(time_t *t)
{
    time_t t_temp;
    l4_uint32_t s, ns;

    // fixme:
    // get KIP time or rdtsc or other time source
    l4_tsc_to_s_and_ns(l4_rdtsc(), &s, &ns);
    // get (cached) system time offset, make sum
    t_temp = s + offset;
    if (t)
        *t = t_temp;
    return t_temp;
}
