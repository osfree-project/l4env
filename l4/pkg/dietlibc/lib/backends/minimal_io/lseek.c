/**
 * \file   dietlibc/lib/backends/minimal_io/lseek.c
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

/* Just a dummy seek function, to make it compile
 *
 */
off_t lseek(int fd, off_t offset, int whence) __THROW
{
    // just accept lseek to stdin, stdout and stderr
    if ((fd != STDIN_FILENO) &&
        (fd != STDOUT_FILENO) &&
        (fd != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }

    switch(whence)
    {
    case SEEK_SET:
        if (offset < 0)
        {
            errno = EINVAL;
            return -1;
        }
        return offset;
    case SEEK_CUR:
    case SEEK_END:
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}
