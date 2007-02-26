
#include <stdio.h>
#ifdef USE_OSKIT
#include <oskit/console.h>
#else
#include <l4/sys/kdebug.h>
#endif
#include "direct.h"
#include "doprnt.h"

#define	PRINTF_BUFMAX 128

struct printf_state 
{
  char buf[PRINTF_BUFMAX];
  unsigned int index;
};

static void
_flush(struct printf_state *state)
{
#ifdef USE_OSKIT
  console_putbytes((const char *) state->buf, state->index);
#else
  outnstring((const char *) state->buf, state->index);
#endif
  state->index = 0;
}

static void
_printf_char(char *arg, int c)
{
  struct printf_state *state = (struct printf_state *) arg;

  if (c == '\n')
    {
      state->buf[state->index] = 0;
      puts(state->buf);
      state->index = 0;
    }
  else if ((c == 0) || (state->index >= PRINTF_BUFMAX))
    {
      _flush(state);
      putchar(c);
    }
  else
    {
      state->buf[state->index] = c;
      state->index++;
    }
}

static int
_vprintf(const char *fmt, va_list args)
{
  struct printf_state state;

  state.index = 0;
  _doprnt(fmt, args, 0, _printf_char, (char *) &state);

  if (state.index != 0)
    _flush(&state);

  return 0;
}

int
putchar(int c)
{
#ifdef USE_OSKIT
  return console_putchar(c);
#else
  outchar(c);
  return 0;
#endif
}

int
puts(const char *s)
{
  while (*s)
    putchar(*s++);
  putchar('\n');
  return 0;
}

int
printf(const char *format,...)
{
  va_list list;
  int err;

  va_start(list, format);
  err = _vprintf(format, list);
  va_end(list);

  return err;
}

