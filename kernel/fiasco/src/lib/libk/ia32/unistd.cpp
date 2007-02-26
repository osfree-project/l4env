// Simplistic Unix interface for use by lib/gmon.cpp.  We support file
// I/O (translated into uuencoded output), sbrk()-like memory
// allocation, and profile() (setting parameters for profiling).

INTERFACE:

#include "types.h"
#include <cstddef>

extern char *pr_base;
extern Address pr_off;
extern size_t pr_size;
extern size_t pr_scale;

IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <panic.h>

#include "kmem_alloc.h"
#include "config.h"
#include "uuencode.h"

#if 0
static bool old_kdb_conn, old_use_serial;
#endif

int 
creat(const char* fname, int mode)
{
  printf("starting to output: %s.uu -- hit Return\n", fname);
#warning why disconnecting the kdb? and fiddle around with serial.
#if 0 
  old_kdb_conn = kdb::connected();
  old_use_serial = console::use_serial;
  kdb::disconnect();

  console::use_serial = false;
#endif
  getchar();
#if 0
  console::use_serial = true;
#endif
  uu_open(fname, mode);

  return 6; 
}

void 
perror(const char *s)
{
  panic (s);
}

ssize_t 
write(int fd, const void *buf, size_t size)
{
  if (fd == 6)
    uu_write((char*) buf, size);
  else if (fd == 2)
    printf((char*) buf);
  else
    panic("assertion failed in gmon.c:write()");

  return size;
}

int 
close(int fd)
{
  assert(fd == 6);
  (void)fd;

  uu_close();
#warning why disconnecting the kdb? and fiddle around with serial.
#if 0 
  console::use_serial = old_use_serial;
  if (old_kdb_conn) 
    kdb::reconnect();
#endif

  return 0;
}
  
void *
sbrk(size_t size)
{
  void *ret = Kmem_alloc::allocator()->alloc((size+Config::PAGE_SIZE-1)/Config::PAGE_SIZE);
  if (ret == 0) 
    ret = (void*)-1;
  else
    memset(ret, 0, size);

  return ret;
}

void 
sbrk_free(void* buf, size_t len)
{
  Kmem_alloc::allocator()->free((len+Config::PAGE_SIZE-1)/Config::PAGE_SIZE, buf);
}

char *pr_base;
Address pr_off;
size_t pr_size;
size_t pr_scale;

int
profil(char *samples, size_t size, Address offset, size_t scale)
{
  pr_base = samples;
  pr_size = size;
  pr_off = offset;
  pr_scale = scale;

  return 0;
}
