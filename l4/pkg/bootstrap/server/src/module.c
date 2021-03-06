#include <stdio.h>
#include "module.h"

void
print_module_name(const char *name, const char *alt_name)
{
  const char *c1, *c2;

  if (!name)
    {
      printf("%s", alt_name);
      return;
    }

  c2 = name;
  while (*c2 != '\0' && *c2 != ' ')
    c2++;
  c1 = c2;
  if (c1 > name)
    c1--;
  while (c1 > name && c2-c1 < 56)
    c1--;

  printf("%.*s", (unsigned)(c2-c1), c1);
}
