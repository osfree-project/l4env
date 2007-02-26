/**
 * \file   libc_backends_l4env/lib/minimal_log_io/write.c
 * \brief  Simple write to stdout and stderr, mapped on LOG_fputs.
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

#include <l4/log/l4log.h>

int write(int fd, const void *buf, size_t count) __THROW
{
    // just accept write to stdout and stderr
    if ((fd == STDOUT_FILENO) || (fd == STDERR_FILENO))
    {
        // ugly, but log has no write()-compatible function
        static char tmp[1024];

        strncpy(tmp, buf, 1023);
        if (count < 1024)
            tmp[count] = 0;
        LOG_fputs(tmp);
        return count;
    }

    // writes to other fds shall fail fast
    errno = EBADF;
    return -1;
}
