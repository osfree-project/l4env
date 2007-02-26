/**
 * \file   dietlibc/lib/backends/simple_sleep/sleep.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include <l4/util/util.h>

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    int milis;

    if (req == NULL)
    {
        errno = EFAULT; // or maybe EINVAL ???
        return -1;
    }

    if (req->tv_nsec < 0 || req->tv_nsec > 999999999 || req->tv_sec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    milis = (req->tv_sec * 1000) + (req->tv_nsec / 1000000);
    l4_sleep(milis);

    return 0;
}
