/*!
 * \file   log/lib/src/log.c
 * \brief  Implementation of the LOG macro
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include <l4/log/l4log.h>
#include <l4/log/log_printf.h>
#include "internal.h"

void LOG_log(const char*function, const char*format,...){
  va_list list;

  lock_printf;

  LOG_printf("%s(): ", function);
  va_start(list,format);
  LOG_vprintf(format,list);
  va_end(list);
  LOG_putchar('\n');

  unlock_printf;
}
