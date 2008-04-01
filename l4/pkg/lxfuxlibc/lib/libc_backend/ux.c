/*!
 * \file   ux.c
 * \brief  open/read/seek/close for UX
 *
 * \date   2008-02-27
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <errno.h>
#include <l4/lxfuxlibc/lxfuxlc.h>
#include <l4/util/util.h>
#include <l4/libc_backends/libfile.h>

int open64(const char *name, int flags, int mode)
{
  int fd = lx_open(name, flags, mode);
  //printf("open64(%s,%d,%d)=%d\n", name, flags, mode, fd);
  errno = fd < 0 ? -2 : 0;
  return fd < 0 ? -1 : fd;
}

int open(const char *name, int flags, int mode)
{
  return open64(name, flags, mode);
}

size_t read(int fd, void *buf, size_t count)
{
  l4_touch_rw(buf, count);
  int r = lx_read(fd, buf, count);
  //printf("read(%d, %p, %d)= %d\n", fd, buf, count,r);
  errno = r < 0 ? r : 0;
  return r < 0 ? -1 : r;
}

unsigned int lseek(int fd, unsigned int offset, int whence)
{
  int r = lx_lseek(fd, offset, whence);
  //printf("lseek(%d, %d, %d)=%d\n", fd, offset, whence, r);
  errno = r < 0 ? r : 0;
  return r < 0 ? -1 : r;
}

int close(int x) {
  int r = lx_close(x);
  return r < 0 ? -1 : 0;
}
