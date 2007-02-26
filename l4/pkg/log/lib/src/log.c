/*!
 * \file   log/lib/src/log.c
 * \brief  Implementation of the LOG macro
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>
#include <l4/log/log_printf.h>
#include "internal.h"

void LOG_log(const char *function, const char *format, ...)
{
  va_list list;
  buffer_t buffer = {0, 0, 0};

  init_buffer(&buffer);

  LOG_printf_buffer(&buffer, "%s(): ", function);
  va_start(list, format);
  LOG_vprintf_buffer(&buffer, format, list);
  va_end(list);
  LOG_putchar_buffer(&buffer, '\n');
}
