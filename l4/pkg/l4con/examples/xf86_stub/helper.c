
#include <stdarg.h>
#include <stdio.h>
#include <l4/sys/types.h>
#include <l4/log/l4log.h>
#include <l4/env/env.h>
#include <xf86.h>

void
LOG_flush(void)
{
}

void
LOG_log(const char *function, const char *format, ...)
{
}

int
printf(const char *format, ...)
{
  va_list list;
  va_start(list, format);
  xf86DrvMsg(0, 5 /*X_ERROR*/, format, list);
  va_end(list);
  return 0;
}

l4_threadid_t
l4env_get_default_dsm(void)
{
  return L4_INVALID_ID;
}
