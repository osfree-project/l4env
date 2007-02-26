
#include "check.h"

#include <l4/env/errno.h>

#include <stdio.h>
#include <stdarg.h>

int
check(int error, const char *format, ...)
{
  if (error)
    {
      va_list list;
      
      printf("Error %d (%s) ", error, l4env_errstr(error));
      va_start(list, format);
      vprintf(format, (oskit_va_list)list);
      va_end(list);
      printf("\n");
    }
  
  return error;
}

