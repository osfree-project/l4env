/**
 * \file   dietlibc/lib/backends/time/settimeofday.c
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
#include <errno.h>

// do nothing for now
int settimeofday(const struct timeval *tv , const struct timezone *tz)
{
    errno = EPERM;
    return -1;
}
