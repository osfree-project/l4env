/* $Id$ */
/**
 * \file	bootstrap/server/src/libc_support.c
 * \brief	Support for C library
 *
 * \date	2004
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

#include <l4/arm_drivers/uart_base.h>
#include <l4/arm_drivers/uart_pl011.h>
#if defined ARCH_arm_isg
# include <l4/arm_drivers/isg/usif.h>
# include <l4/arm_drivers/isg/usart.h>
#endif
#include <l4/arm_drivers_c/hw.h>
#include <l4/crtx/crt0.h>

extern "C" int have_hercules(void);
int have_hercules(void)
{
  return 0;
}

static union
{
#if defined ARCH_arm_isg
  char u1[sizeof(L4::ISG_usif)];
  char u2[sizeof(L4::ISG_usart)];
#endif
  char u3[sizeof(L4::Uart_pl011)];
} _uart;

inline void *operator new(size_t, void *p) throw()
{ return p; }


inline L4::Uart *uart()
{
  return reinterpret_cast<L4::Uart*>(&_uart);
}

extern char _bss_start[], _bss_end[];
extern "C" void startup(void);

extern "C" void __main(void);

void __main(void)
{
  memset(_bss_start, 0, (char *)&crt0_stack_low - _bss_start);
  memset((char *)&crt0_stack_high, 0, _bss_end - (char *)&crt0_stack_high);
  hw_init();

#if defined ARCH_arm
# if defined ARCH_arm_integrator
  new (&_uart) L4::Uart_pl011(1,1);
  uart()->startup(0x16000000);
# elif defined ARCH_arm_rv
  new (&_uart) L4::Uart_pl011(36,36);
  uart()->startup(0x10009000);
# elif defined ARCH_ARM_isg_3
  new (&_uart) L4::ISG_usif(133,134);
  uart()->startup(0xf7500000);
# elif defined (ARCH_ARM_isg_2) || defined (ARCH_ARM_isg_1)
  new (&_uart) L4::ISG_usart(6,6);
  uart()->startup(0xf1000000);
# else
#   error There exists no driver in libuart for the given driver type
# endif
#endif
  startup();
  while(1);
}

ssize_t
write(int fd, const void *buf, size_t count)
{
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
  do
    c = uart()->get_char(0);
  while (c == -1);
  return c;
}

#ifdef USE_DIETLIBC
off_t lseek(int fd, off_t offset, int whence)
#else
off64_t lseek64(int fd, off64_t offset, int whence)
#endif
{
  return 0;
}
#if 0
  // just accept lseek to stdin, stdout and stderr
  if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO)
    {
      errno = EBADF;
      return -1;
    }

  switch (whence)
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
#endif

void *__dso_handle = &__dso_handle;

/** for crt0 */
extern "C" void l4util_reboot_arch(void) __attribute__((noreturn));
extern "C" void reboot(void) __attribute__((noreturn));
void reboot(void)
{
  l4util_reboot_arch();
}

extern "C" void __attribute__((noreturn))
_exit(int rc)
{
  printf("\n\033[1mKey press reboots...\033[m\n");
  getchar();
  printf("Rebooting.\n\n");
  reboot();
}

/** for dietlibc, atexit */
extern "C" void __thread_doexit(int rc);
void __thread_doexit(int rc)
{
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
  exit(1);
}

