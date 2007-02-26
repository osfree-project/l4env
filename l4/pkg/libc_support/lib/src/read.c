#include <unistd.h>
#include <errno.h>
#include "l4/env_support/getchar.h"

ssize_t read(int fd, void *buf, size_t count)
{
  if (fd == STDIN_FILENO)
    {
      /* only 1 character available */
      ((char*)buf)[0] = getchar();
      return 1;
    }

  errno = EBADF;
  return -1;
}
