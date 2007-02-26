/**
 * \file   dietlibc/lib/backends/minimal_io/write.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <l4/sys/kdebug.h>

ssize_t write(int fd, const void *buf, size_t count) __THROW
{
    // just accept write to stdout and stderr
    if ((fd == STDOUT_FILENO) || (fd == STDERR_FILENO))
    {
        outnstring((const char*)buf, count);
        return count;
    }

    // writes to other fds shall fail fast
    errno = EBADF;
    return -1;
}
