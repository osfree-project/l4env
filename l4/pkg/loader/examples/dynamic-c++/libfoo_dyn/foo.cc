#include "stdio.h"
#include "foo.h"

extern "C" void bar_in_binary(void);

unsigned
foo_add(unsigned a, unsigned b)
{
  return a + b;
}

void
foo_show(void)
{
  printf("This is a output from the library function foo_show\n");
  bar_in_binary();
}
