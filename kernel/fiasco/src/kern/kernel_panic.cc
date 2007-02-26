#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <panic.h>

#include "kernel_console.h"
#include "simpleio.h"
#include "terminate.h"


void __assert_fail (const char *__assertion, const char *__file,
		    unsigned int __line, const char *__function)
{
  Kconsole::console()->gzip_disable();

  printf("Assertion failed: %s:%i in function %s '%s'\n",__file,
	 __line,__function,__assertion);

  terminate(1);
}

void panic (const char *format, ...)
{
  Kconsole::console()->gzip_disable();

  va_list args;

  putstr("\033[1mPanic: ");
  va_start (args, format);
  vprintf  (format, args);
  va_end   (args);
  putstr("\033[m");

  terminate (EXIT_FAILURE);
}

