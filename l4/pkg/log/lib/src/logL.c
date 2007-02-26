/*!
 * \file   log/lib/src/logL.c
 * \brief  
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include "internal.h"
#include <l4/log/log_printf.h>
#ifdef __L4__
#include <l4/sys/syscalls.h>
#else
#include <unistd.h>
#endif
#include <l4/log/l4log.h>

void LOG_logL(const char*file, int line, const char*function,
	      const char*format,...){
#ifdef __L4__
    l4_threadid_t id=l4_myself();
#endif
  
    va_list list;

    lock_printf;

    LOG_printf("%s:%d:%s() ", file, line, function);
    LOG_printf(
#ifdef __L4__
             "[%X.%X]: ",id.id.task,id.id.lthread);
#else
             "[%d]: ",getpid());
#endif
    va_start(list,format);
    LOG_vprintf(format,list);
    va_end(list);
    LOG_putchar('\n');

    unlock_printf;
}
