/*!
 * \file   log/lib/src/printf.c
 * \brief  mapping of printf and friends
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork_loeser@inf.tu-dresden.de>
 *
 * This file implements: printf, fprintf, vprintf, vfprintf, putchar,
 *			 puts, fputs
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>
#include <l4/log/log_printf.h>
#include "internal.h"

int vprintf(const char *format, LOG_va_list list)
{
  return LOG_vprintf(format, list);
}

int vfprintf(FILE *stream, const char *format, LOG_va_list list)
{
  return LOG_vprintf(format, list);
}

int printf(const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = LOG_vprintf(format, list);
  va_end(list);
  return err;
}

int fprintf(FILE *__stream, const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = LOG_vprintf(format, list);
  va_end(list);
  return err;
}

#if 0
/* conflicts with oskit10_freebsd environment */
int fflush(FILE *__stream);
int fflush(FILE *__stream)
{
  LOG_flush();
  return 0;
}
#endif

#ifndef putc
int putc(int c, FILE *stream)
{
  LOG_putchar(c);
  return c;
}
#endif

/* disable dietlibc putchar preprocessor convertion */
#undef putchar
int putchar(int c)
{
  LOG_putchar(c);
  return c;
}

#undef fputc
int fputc(int c, FILE *stream)
{
  LOG_putchar(c);
  return c;
}

int puts(const char *s)
{
  return LOG_puts(s);
}

int fputs(const char *s, FILE *stream)
{
  return LOG_fputs(s);
}

