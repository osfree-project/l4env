/*!
 * \file   log/lib/src/log_printf.c
 * \brief  L4-specific printf-routines: break lines, prepend with logtag
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork_loeser@inf.tu-dresden.de>
 *
 * This file implements: LOG_printf, LOG_vprintf, LOG_putchar, LOG_puts,
 *			 LOG_printf_flush
 * 
 *
 * Locking: We use a local line-buffer (buffer) for our printf implementation,
 *          which is locked to make printf itself atomar.
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>
#include "internal.h"
#include <l4/log/log_printf.h>
#include <limits.h>

static int log_cont=0;	/* Message split to multiple lines ? */
static int bindex = 0;	/* current index in buffer */
static int printed;

/* the log-buffer */
static char buffer[LOG_BUFFERSIZE];

/* and the global lock */
#ifdef __USE_L4WQLOCKS__
static wq_lock_queue_base lock_queue={NULL};

/* hackish, I admit */
#define lock_buffer() wq_lock_queue_elem lock; wq_lock_lock(&lock_queue, &lock)
#define unlock_buffer() wq_lock_unlock(&lock_queue, &lock)
#else
#define lock_buffer()  do{}while(0)
#define unlock_buffer() do{}while(0)
#endif

#if LOG_BUFFERSIZE<=12
#error LOG_BUFFERSIZE to small (need 10 bytes prefix and 2 bytes postfix)
#endif

static void flush(void){
  buffer[bindex++]=0;
  LOG_outstring(buffer);
  bindex = 0;
}

extern inline void init_buffer(void);
extern inline void init_buffer(void){
    const char*p = LOG_tag;
    
    while(*p && bindex<8) buffer[bindex++]=*p++;
    while(bindex<8) buffer[bindex++]=' ';
    buffer[bindex++] = log_cont?':':'|';
    buffer[bindex++] = ' ';
}

/*!\brief print a character
 *
 * Used as callback for _doprnt and within this file.
 */
static void printchar(char*arg, char c){
  if(bindex==0){
    init_buffer();
  }
  if(c=='\n'){
    log_cont=0;
    buffer[bindex++]='\n';
    printed++;
    flush();
    return;
  }
  if(bindex>=LOG_BUFFERSIZE-2){
    log_cont = 1;
    buffer[bindex++] = '\n';
    flush();
    init_buffer();
  }
  buffer[bindex++] = c;
  printed++;
}


int LOG_vprintf(const char*format, LOG_va_list list){
  int i;

  lock_buffer();

  if(format){
    printed = 0;	// counter
    /* use external _doprnt from the oskit */
    LOG_doprnt(format, list, 16, printchar, (char *) &buffer);
    i = printed;	// save it, because it may be destroyed after unlock
  } else {
    flush();
    i=0;
  }

  unlock_buffer();
  return i;
}

int LOG_printf(const char *format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vprintf(format, list);
    va_end(list);
    return err;
}

int LOG_putchar(int c){
    lock_buffer();
    printchar(0, c);
    unlock_buffer();
    return c;
}

int LOG_puts(const char*s){
    lock_buffer();
    while(*s)printchar(0, *s++);
    printchar(0, '\n');
    unlock_buffer();
    return 0;
}

int LOG_fputs(const char*s){
    lock_buffer();
    while(*s)printchar(0, *s++);
    unlock_buffer();
    return 0;
}


typedef struct{
    int maxlen;
    char*ptr;
} desc_t;

/*!\brief print a character to a buffer
 *
 * Used as callback for _doprnt
 */
static void printchar_string(char*arg, char c){
    if(--((desc_t*)arg)->maxlen>0){
	*((desc_t*)arg)->ptr++=c;
    } else {
	if(((desc_t*)arg)->maxlen==0) *((desc_t*)arg)->ptr=0;
	((desc_t*)arg)->ptr++;
    }
}

int LOG_vsprintf(char *s, const char*format, LOG_va_list list){
    desc_t desc;

    if(format){
	desc.maxlen=INT_MAX;
	desc.ptr = s;
	/* use external _doprnt from the oskit */
	LOG_doprnt(format, list, 0, printchar_string, (char*)&desc);
	*desc.ptr=0;
	return desc.ptr - s;
    } else {
	return 0;
    }
}

int LOG_vsnprintf(char *s, unsigned size,
		  const char*format, LOG_va_list list){
    desc_t desc;

    if(format){
	desc.maxlen=size;
	desc.ptr = s;
	/* use external _doprnt from the oskit */
	LOG_doprnt(format, list, 0, printchar_string, (char*)&desc);
	if(desc.maxlen>0) *desc.ptr = 0;
	return desc.ptr - s;
    } else {
	return 0;
    }
}

int LOG_sprintf(char *s, const char *format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vsprintf(s, format, list);
    va_end(list);
    return err;
}

int LOG_snprintf(char *s, unsigned size, const char *format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vsnprintf(s, size, format, list);
    va_end(list);
    return err;
}


void LOG_printf_flush(void){
    lock_buffer();
    flush();
    unlock_buffer();
}
