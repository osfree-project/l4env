/*!
 * \file   log/lib/src/logL.c
 * \brief  Very verbose logging including thread-IDs
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/log_printf.h>
#ifdef __L4__
#include <l4/sys/syscalls.h>
#else
#include <unistd.h>
#endif
#include <l4/log/l4log.h>
#include "internal.h"

void LOG_logL(const char *file, int line, const char *function,
              const char *format,...)
{
#ifdef __L4__
  l4_threadid_t id = l4_myself();
#endif
  va_list list;
  buffer_t buffer = {0, 0, 0};
  init_buffer(&buffer);

#ifdef __L4__
  LOG_printf_buffer(&buffer,
                    "[%X.%X] %s:%d:%s():\n ", id.id.task, id.id.lthread,
                    LOG_filename(file), line, function);
#else
  LOG_printf_buffer(&buffer,
                    "[%d] %s:%d:\n%s(): ", getpid(),
                    LOG_filename(file), line, function);
#endif
  va_start(list, format);
  LOG_vprintf_buffer(&buffer, format,list);
  va_end(list);
  LOG_putchar_buffer(&buffer, '\n');
}
