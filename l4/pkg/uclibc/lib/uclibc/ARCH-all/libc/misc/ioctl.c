#include <sys/ioctl.h>
#include <errno.h>

int ioctl(int fd, unsigned long int request, ...)
{
  return -ENOTTY;
}
