/**
 * \file	bootstrap/server/src/libc_support.c
 * \brief	Support for C library
 *
 * \date	2004-2008
 * \author	Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>

#include <l4/cxx/iostream.h>

#include "support.h"


static L4::Uart *stdio_uart;

L4::Uart *uart() 
{ return stdio_uart; }

void set_stdio_uart(L4::Uart *uart)
{ stdio_uart = uart; }


// IO Stream backend
namespace {

  class BootstrapIOBackend : public L4::IOBackend
  {
  protected:
    void write(char const *str, unsigned len);
  };

  void BootstrapIOBackend::write(char const *str, unsigned len)
  {
    ::write(STDOUT_FILENO, str, len);
  }

  namespace {
    BootstrapIOBackend iob;
  };
};

namespace L4 {
  BasicOStream cout(&iob);
  BasicOStream cerr(&iob);
};

extern char _bss_start[], _bss_end[];

extern "C" void startup(unsigned long p1, unsigned long p2, unsigned long p3);

#ifdef ARCH_arm
#include <l4/crtx/crt0.h>
extern "C" void __main(void);
void __main(void)
{
  unsigned long r;
  asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r" (r) : : "memory");
  r |= 2; // alignment check on
  asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r" (r) : "memory");

  memset(_bss_start, 0, (char *)&crt0_stack_low - _bss_start);
  memset((char *)&crt0_stack_high, 0, _bss_end - (char *)&crt0_stack_high);

  platform_init();

  startup(0, 0, 0);
  while(1)
    ;
}
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
extern "C" void __main(unsigned long p1, unsigned long p2, unsigned long p3);
void __main(unsigned long p1, unsigned long p2, unsigned long p3)
{
  platform_init();
  startup(p1, p2, p3);
}
#endif

ssize_t
write(int fd, const void *buf, size_t count)
{
  if (!uart())
    return 0;
  // just accept write to stdout and stderr
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
      uart()->write((const char*)buf, count);
      return count;
    }

  // writes to other fds shall fail fast
  errno = EBADF;
  return -1;
}

#undef getchar
int
getchar(void)
{
  int c;
  if (!uart())
    return -1;

  do
    c = uart()->get_char(0);
  while (c == -1)
    ;
  return c;
}

off64_t lseek64(int fd, off64_t offset, int whence)
{
  return 0;
}

void *__dso_handle = &__dso_handle;

/** for crt0 */
extern "C" void l4util_reboot_arch(void) __attribute__((noreturn));
extern "C" void reboot(void) __attribute__((noreturn));
void reboot(void)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  l4util_reboot_arch();
#else
  while (1)
    ;
#endif
}

extern "C" void __attribute__((noreturn))
_exit(int rc)
{
  printf("\n\033[1mKey press reboots...\033[m\n");
  getchar();
  printf("Rebooting.\n\n");
  reboot();
}

/** for assert */
void
abort(void) throw()
{
  _exit(1);
}

extern "C"
void
panic(const char *fmt, ...);

void
panic(const char *fmt, ...)
{
  va_list v;
  va_start (v, fmt);
  vprintf(fmt, v);
  va_end(v);
  putchar('\n');
  _exit(1);
}
