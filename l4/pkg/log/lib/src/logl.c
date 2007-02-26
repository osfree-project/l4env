/*!
 * \file   log/lib/src/logl.c
 * \brief  
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include <l4/log/l4log.h>
#include "internal.h"
#include <l4/log/log_printf.h>

void LOG_logl(const char*file, int line, const char*function,
	      const char*format,...){
    va_list list;
  
    lock_printf;

    LOG_printf("%s:%d:%s(): ",file, line, function);
    va_start(list,format);
    LOG_vprintf(format,list);
    va_end(list);
    LOG_putchar('\n');

    unlock_printf;
}
