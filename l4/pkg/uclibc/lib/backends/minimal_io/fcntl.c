#include <stdio.h>
#include <errno.h>

int fcntl(int fd, int cmd);

int fcntl(int fd, int cmd)
{
  printf("fcntl() called: unimplemented!\n");
  errno = EINVAL;
  return -EINVAL;
}

