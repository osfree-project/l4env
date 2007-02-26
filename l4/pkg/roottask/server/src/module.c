#include <stdio.h>
#include "module.h"

void
print_module_name(const char *name, int length, int max_length)
{
  const char *c;
  int i;

  /* print module names without parameters. */
  for (c=name; *c!='\0' && *c!=' '; c++)
    ;
  /* right justify module name */
  for (i=0; c>name && i<max_length; c--, i++)
    ;

  printf("%-*.*s", length, i, c);
}
