/*
 * Could/Should be moved to libc somewhere.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <l4/env_support/panic.h>

void panic(const char *fmt, ...)
{
  va_list v;

  va_start(v, fmt);
  vprintf(fmt, v);
  va_end(v);

  puts(""); // newline

  exit(1);
}
