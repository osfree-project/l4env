#include <stdarg.h>
#include <stdio.h>

#include <l4/log/log_printf.h>
#include "internal.h"
#include "doprnt.h"

void printf_flush(void);

static l4_uint8_t linebuf[CONTXT_LINEBUF_SIZE+1];
static int linebuf_idx;
static int printed;

static void
flush_linebuf(void)
{
  if (__init)
    {
      contxt_write(linebuf, linebuf_idx);
      linebuf_idx = 0;
    }
  else
    {
      linebuf[linebuf_idx] = 0;
      LOG_printf("%s", linebuf);
    }
  linebuf_idx = 0;
}


static void
printchar(char*arg, int c)
{
  if (linebuf_idx >= (CONTXT_LINEBUF_SIZE - 1))
    flush_linebuf();

  linebuf[linebuf_idx++] = c;
  printed++;
}


int 
#ifdef USE_OSKIT
vprintf(const char*format, oskit_va_list list)
#else
vprintf(const char*format, va_list list)
#endif
{
  int i = 0;

  if (format)
    {
      printed = 0;
      _doprnt(format, list, 0, (void (*)(char*,char))printchar, 0);
      i = printed;
    }

  if (linebuf_idx != 0)
    flush_linebuf();

  return i;
}


int 
printf(const char *format,...)
{
  va_list list;
  int err;
 
  va_start(list, format);
  err=vprintf(format, list);
  va_end(list);
  return err;
}


void
printf_flush(void)
{}


int 
fprintf(FILE *__stream, const char *format, ...)
{
  va_list list;
  int err;

  va_start(list, format);
  err=vprintf(format, list);
  va_end(list);
  return err;
}

