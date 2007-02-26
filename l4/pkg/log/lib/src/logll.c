/*!
 * \file   log/lib/src/logl.c
 * \brief  verbose logging
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include <l4/log/l4log.h>
#include "internal.h"
#include <l4/log/log_printf.h>

void LOG_logl(const char *file, int line, const char *function,
              const char *format,...)
{
  va_list list;
  buffer_t buffer = {0, 0, 0};
  init_buffer(&buffer);

  LOG_printf_buffer(&buffer,
                    "%s:%d:%s():\n ", LOG_filename(file), line, function);
  va_start(list,format);
  LOG_vprintf_buffer(&buffer, format, list);
  va_end(list);
  LOG_putchar_buffer(&buffer, '\n');
}
