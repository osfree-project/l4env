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

static buffer_t global_buffer;

/* and the global lock */
#ifdef __USE_L4WQLOCKS__
#include <l4/util/lock_wq.h>
static l4util_wq_lock_queue_base_t lock_queue = { NULL };

/* hackish, I admit */
#define lock_buffer()   l4util_wq_lock_queue_elem_t lock;          \
                        l4util_wq_lock_lock(&lock_queue, &lock)
#define unlock_buffer() l4util_wq_lock_unlock(&lock_queue, &lock)
#else
#define lock_buffer()   do {} while(0)
#define unlock_buffer() do {} while(0)
#endif

#if LOG_BUFFERSIZE <= 12
#error LOG_BUFFERSIZE to small (need 10 bytes prefix and 2 bytes postfix)
#endif

static void flush(buffer_t *b)
{
  b->buffer[b->bindex++] = 0;
  LOG_outstring(b->buffer);
  b->bindex = 0;
}

/*!\brief print a character
 *
 * Used as callback for _doprnt and within this file.
 */
static void printchar(char *arg, char c)
{
  buffer_t *b = (buffer_t*)arg;

  if (b->bindex == 0)
      init_buffer(b);

  if (c == '\n')
    {
      b->log_cont = 0;
      b->buffer[b->bindex++] = '\n';
      b->printed++;
      flush(b);
      return;
    }
  if (b->bindex >= LOG_BUFFERSIZE - 2)
    {
      b->log_cont = 1;
      b->buffer[b->bindex++] = '\n';
      flush(b);
      init_buffer(b);
    }
  b->buffer[b->bindex++] = c;
  b->printed++;
}


int LOG_vprintf_buffer(buffer_t *b, const char *format, LOG_va_list list)
{
  if (format)
    {
      /* use external _doprnt from the oskit */
      LOG_doprnt(format, list, 16, printchar, (char *)b);
      return b->printed;
    }
  return 0;
}

int LOG_printf_buffer(buffer_t *b, const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = LOG_vprintf_buffer(b, format, list);
  va_end(list);
  return err;
}

int LOG_putchar_buffer(buffer_t *b, int c)
{
  printchar((char*)b, c);
  return c;
}

int LOG_vprintf(const char*format, LOG_va_list list)
{
  if (format)
    {
      int i;

      lock_buffer();
      /* use external _doprnt from the oskit */
      global_buffer.printed = 0;
      LOG_doprnt(format, list, 16, printchar, (char *)&global_buffer);
      i = global_buffer.printed;
      unlock_buffer();
      return i;
   }

  return 0;
}

int LOG_printf(const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = LOG_vprintf(format, list);
  va_end(list);
  return err;
}

int LOG_putchar(int c)
{
  lock_buffer();
  printchar((char*)&global_buffer, c);
  unlock_buffer();
  return c;
}

int LOG_puts(const char *s)
{
  lock_buffer();
  while (*s)
    printchar((char*)&global_buffer, *s++);
  printchar((char*)&global_buffer, '\n');
  unlock_buffer();
  return 0;
}

int LOG_fputs(const char *s)
{
  lock_buffer();
  while (*s)
    printchar((char*)&global_buffer, *s++);
  unlock_buffer();
  return 0;
}


typedef struct {
    int   maxlen;
    char *ptr;
} desc_t;

/*!\brief print a character to a buffer
 *
 * Used as callback for _doprnt
 */
static void printchar_string(char *arg, char c)
{
  if (--((desc_t*)arg)->maxlen > 0)
    *((desc_t*)arg)->ptr++ = c;
  else
    {
      if (((desc_t*)arg)->maxlen == 0)
        *((desc_t*)arg)->ptr = 0;
      ((desc_t*)arg)->ptr++;
    }
}

int LOG_vsprintf(char *s, const char*format, LOG_va_list list)
{
  desc_t desc;

  if (format)
    {
      desc.maxlen = INT_MAX;
      desc.ptr = s;
      /* use external _doprnt from the oskit */
      LOG_doprnt(format, list, 0, printchar_string, (char*)&desc);
      *desc.ptr = 0;
      return desc.ptr - s;
    }

  return 0;
}

int LOG_vsnprintf(char *s, unsigned size,
                 const char *format, LOG_va_list list)
{
  desc_t desc;

  if (format)
    {
      desc.maxlen = size;
      desc.ptr = s;
      /* use external _doprnt from the oskit */
      LOG_doprnt(format, list, 0, printchar_string, (char*)&desc);
      if (desc.maxlen > 0)
        *desc.ptr = 0;
      return desc.ptr - s;
    }

  return 0;
}

int LOG_sprintf(char *s, const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = LOG_vsprintf(s, format, list);
  va_end(list);
  return err;
}

int LOG_snprintf(char *s, unsigned size, const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = LOG_vsnprintf(s, size, format, list);
  va_end(list);
  return err;
}


void LOG_printf_flush(void)
{
  lock_buffer();
  flush(&global_buffer);
  unlock_buffer();
}
