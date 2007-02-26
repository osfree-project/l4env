/*!
 * \file   log/lib/src/logI.c
 * \brief  
 *
 * \date   10/16/2000
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *	   Jork Loeser <jork@os.inf.tu-dresden.de>
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

void LOG_logI(const char *file, int line, const char *func,
	      const char *format,...){
#ifdef __L4__
    l4_threadid_t id=l4_myself();
#endif
  
    va_list list;

    lock_printf;

    LOG_printf("%s:%d (%s,",file,line,func);
    LOG_printf(
#ifdef __L4__
             "%x.%x): ",id.id.task,id.id.lthread);
#else
             "%d): ",getpid());
#endif
    va_start(list,format);
    LOG_vprintf(format,list);
    va_end(list);
    LOG_putchar('\n');

    unlock_printf;
}
