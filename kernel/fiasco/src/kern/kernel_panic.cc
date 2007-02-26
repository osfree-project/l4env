#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <panic.h>

#include "terminate.h"

void __assert_fail (const char *__assertion, const char *__file,
		    unsigned int __line, const char *__function)
{
  printf("Assertion failed: %s:%i in function %s '%s'\n",__file,
	 __line,__function,__assertion);

  terminate(1);
}

void panic (const char *format, ...) {

  va_list args;
  
  va_start (args, format);
  vprintf  (format, args);
  va_end   (args);


  terminate (EXIT_FAILURE);
}
