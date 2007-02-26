/* $Id$ */
/**
 * \file	bootstrap/server/src/libc_support.c
 * \brief	Support for C library
 * 
 * \date	2004
 * \author	Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>

#include "base_critical.h"
#if defined(ARCH_x86) || defined(ARCH_amd64)
#include "ARCH-x86/serial.h"
#include <l4/util/cpu.h>
#include <l4/util/port_io.h>
#endif
#ifdef ARCH_arm
#include <l4/arm_drivers_c/uart.h>
#include <l4/arm_drivers_c/hw.h>
#include <l4/crtx/crt0.h>
#endif
#include <l4/env_support/panic.h>

int have_hercules(void);
int have_hercules(void)
{
  return 0;
}

#ifdef ARCH_arm
extern char _bss_start[], _bss_end[];
extern void startup(void);

void __main(void)
{
  hw_init();
  memset(_bss_start, 0, (char *)&crt0_stack_low - _bss_start);
  memset((char *)&crt0_stack_high, 0, _bss_end - (char *)&crt0_stack_high);
  uart_startup(uart_base(), 0);
  uart_change_mode(uart_get_mode(UART_MODE_TYPE_8N1), 115200);
  startup();
  while(1);
}
#endif


#if defined(ARCH_x86) || defined(ARCH_amd64)
/** VGA console output */
static void
direct_cons_putchar(unsigned char c)
{
  static int ofs = -1, esc, esc_val, attr = 0x07;
  unsigned char *vidbase = (unsigned char*)0xb8000;

  base_critical_enter();

  if (ofs < 0)
    {
      /* Called for the first time - initialize.  */
      ofs = 80*2*24;
      direct_cons_putchar('\n');
    }

  switch (esc)
    {
    case 1:
      if (c == '[')
	{
	  esc++;
	  goto done;
	}
      esc = 0;
      break;

    case 2:
      if (c >= '0' && c <= '9')
	{
	  esc_val = 10*esc_val + c - '0';
	  goto done;
	}
      if (c == 'm')
	{
	  attr = esc_val ? 0x0f : 0x07;
	  goto done;
	}
      esc = 0;
      break;
    }

  switch (c)
    {
    case '\n':
      bcopy(vidbase+80*2, vidbase, 80*2*24);
      bzero(vidbase+80*2*24, 80*2);
      /* fall through... */
    case '\r':
      ofs = 0;
      break;

    case '\t':
      ofs = (ofs + 8) & ~7;
      break;

    case '\033':
      esc = 1;
      esc_val = 0;
      break;

    default:
      /* Wrap if we reach the end of a line.  */
      if (ofs >= 80)
	direct_cons_putchar('\n');

      /* Stuff the character into the video buffer. */
	{
	  volatile unsigned char *p = vidbase + 80*2*24 + ofs*2;
	  p[0] = c;
	  p[1] = attr;
	  ofs++;
	}
      break;
    }

done:
  base_critical_leave();
}

/** Poor man's getchar, only returns raw scan code. We don't need to know
 * _which_ key was pressed, we only want to know _if_ a key was pressed. */
static int
direct_cons_try_getchar(void)
{
  unsigned status, scan_code;

  base_critical_enter();

  l4util_cpu_pause();

  /* Wait until a scan code is ready and read it. */
  status = l4util_in8(0x64);
  if ((status & 0x01) == 0)
    {
      base_critical_leave();
      return -1;
    }
  scan_code = l4util_in8(0x60);

  /* Drop mouse events */
  if ((status & 0x20) != 0)
    {
      base_critical_leave();
      return -1;
    }

  base_critical_leave();
  return scan_code;
}
#endif /* ARCH_x86 */

static inline void
direct_outnstring(const char *buf, int count)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  while (count--)
    {
      direct_cons_putchar(*buf);
      com_cons_putchar(*buf++);
    }
#endif
#ifdef ARCH_arm
  uart_write(buf, count);
#endif
}

ssize_t
write(int fd, const void *buf, size_t count)
{
  // just accept write to stdout and stderr
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
      direct_outnstring((const char*)buf, count);
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
    {
#if defined ARCH_x86 || defined ARCH_amd64
      c = com_cons_try_getchar();
      if (c == -1)
	c = direct_cons_try_getchar();
      l4util_cpu_pause();
#endif
#ifdef ARCH_arm
      c = uart_getchar(0);
#endif
    }
  while (c == -1);
  return c;
}

#ifdef USE_DIETLIBC
off_t lseek(int fd, off_t offset, int whence)
#else
off64_t lseek64(int fd, off64_t offset, int whence)
#endif
{
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

/** for crt0 */
void reboot(void) __attribute__((noreturn));
void reboot(void)
{
  extern void l4util_reboot_arch(void) __attribute__((noreturn));
  l4util_reboot_arch();
}

void __attribute__((noreturn))
_exit(int rc)
{
  printf("\n\033[1mKey press reboots...\033[m\n");
  getchar();
  printf("Rebooting.\n\n");
  reboot();
}

/** for dietlibc, atexit */
void __thread_doexit(int rc);
void __thread_doexit(int rc)
{
}

/** for assert */
void
abort(void)
{
  _exit(1);
}

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
