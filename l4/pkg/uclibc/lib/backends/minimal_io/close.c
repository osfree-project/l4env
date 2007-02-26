#include <stdio.h>
#include <errno.h>

int close(int fd);
  
int close(int fd)
{
  printf("close() called: unimplemented!\n");
  errno = EINVAL;
  return -EINVAL;
}

