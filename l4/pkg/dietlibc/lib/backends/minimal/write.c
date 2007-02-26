#include <unistd.h>
#include <errno.h>

#include <l4/sys/kdebug.h>

int write(int fd, const void *buf, size_t count) __THROW
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
