/*!
 * \file   log/lib/include/internal.h
 * \brief  Internal prototypes for loglib
 *
 * \date   02/14/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#ifndef __LOG_LIB_INCLUDE_INTERNAL_H_
#define __LOG_LIB_INCLUDE_INTERNAL_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __L4__
/* L4 part */

/* under L4 we might use the logserver */
#include "log_comm.h"

/* if we want use locks, we need some declarations */
#ifdef __USE_L4WQLOCKS__

#include "lock.h"

extern wq_lock_queue_base LOG_lock_queue;

/* A delicate macro to make multiple printf-calls atomic
 * It has no '()' to make it apear special in the source code.
 */
#define lock_printf wq_lock_queue_elem lock; wq_lock_lock(&LOG_lock_queue, &lock)
#define unlock_printf wq_lock_unlock(&LOG_lock_queue, &lock)
#else
#warning Your should use locking under L4
#define lock_printf
#dedine unlock_printf
#endif

/* timeout for nameserver to return id of logserver */
#define LOG_TIMEOUT_NAMESERVER 3000

#else
/* Linux part*/

/* we need to define buffersize, because linux doesnt need interaction with
   the logserver and doesnt include log_comm.h */
#define LOG_BUFFERSIZE 81
#define lock_printf
#define unlock_printf
#endif
/* Both use this */

extern void (*LOG_outstring)(const char*log_message);

#define LOG_NICE_BUFFER(buffer,maxlen,printedlen)		\
  do{								\
    if(printedlen==-1 || (printedlen==maxlen-1 &&		\
                          buffer[maxlen-2]!='\n')){		\
      strcpy(buffer+maxlen-5,"...\n");				\
    }else if(buffer[printedlen-1]!='\n') strcat(buffer,"\n");	\
  }while(0)

extern void LOG_printf_flush(void);
extern void LOG_doprnt(register const char*,va_list, int,
		        void(*)(char*,char), char*);

#endif /* __INTERNAL_H_ */
