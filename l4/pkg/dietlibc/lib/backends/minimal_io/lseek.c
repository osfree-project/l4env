#include <unistd.h>
#include <errno.h>

/* Just a dummy seek function, to make it compile
 *
 */
off_t lseek(int fd, off_t offset, int whence) __THROW
{
    // just accept lseek to stdin, stdout and stderr
    if ((fd != STDIN_FILENO) &&
        (fd != STDOUT_FILENO) &
        (fd != STDERR_FILENO))
    {
        errno = EBADF;
        return (off_t)-1;
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
        break;
    case SEEK_CUR:
        return 0;
        break;
    case SEEK_END:
        return 0;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
}
