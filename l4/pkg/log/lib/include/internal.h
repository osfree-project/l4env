/*!
 * \file   log/lib/include/internal.h
 * \brief  Internal prototypes for loglib
 *
 * \date   02/14/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LOG_LIB_INCLUDE_INTERNAL_H_
#define __LOG_LIB_INCLUDE_INTERNAL_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <l4/log/l4log.h>

#ifdef __L4__
/* L4 part */

#include <l4/sys/ipc.h>

/* under L4 we might use the logserver */
#include "log_comm.h"

/* timeout for nameserver to return id of logserver */
#define LOG_TIMEOUT_NAMESERVER 3000

extern int check_server(void);
extern l4_threadid_t log_server;
extern int initialized;

#else
/* Linux part*/

/* we need to define buffersize, because linux doesnt need interaction with
   the logserver and doesnt include log_comm.h */
#define LOG_BUFFERSIZE 81
#define lock_printf
#define unlock_printf

#endif
/* Both use this */

typedef struct {
    int log_cont;		/* Message split to multiple lines ? */
    int bindex;			/* current index in buffer */
    int printed;		/* number of chars printed. Used in vsprintf */
    char buffer [LOG_BUFFERSIZE]; /* the log-buffer */
} buffer_t;

extern inline void init_buffer(buffer_t *b);

extern void (*LOG_outstring)(const char *log_message);

#define LOG_NICE_BUFFER(buffer, maxlen, printedlen)		\
  do{								\
    if (printedlen==-1 || (printedlen==maxlen-1 &&		\
                           buffer[maxlen-2]!='\n')) {		\
      strcpy(buffer+maxlen-5, "...\n");				\
    } else if(buffer[printedlen-1]!='\n') strcat(buffer, "\n");	\
  } while(0)

void LOG_printf_flush(void);
void LOG_doprnt(register const char *, va_list, int,
                void(*)(char *, char), char *);

int LOG_vprintf_buffer(buffer_t *b, const char *format,
                              LOG_va_list list);
int LOG_printf_buffer(buffer_t *b, const char *format, ...);
int LOG_putchar_buffer(buffer_t *b, int c);

/* extract filename */
extern inline const char* LOG_filename(const char *name);

/* Implementation */

extern inline void init_buffer(buffer_t *b)
{
  const char*p = LOG_tag;

  while (*p && b->bindex < 8)
    b->buffer[b->bindex++] = *p++;
  while (b->bindex < 8)
    b->buffer[b->bindex++] = ' ';
  b->buffer[b->bindex++] = b->log_cont ? ':' : '|';
  b->buffer[b->bindex++] = ' ';
}

extern inline const char* LOG_filename(const char *name)
{
  char *f = strstr(name, "pkg/");
  return (f != NULL) ? f + 4 : name;
}

#endif /* __INTERNAL_H_ */
