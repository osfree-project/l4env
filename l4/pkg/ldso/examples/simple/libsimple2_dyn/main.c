#include <stdio.h>

extern void bar_in_binary(void);

void foo_in_library_2(void);

void
foo_in_library_2(void)
{
  printf("This is output from foo_in_library_2()\n");
  bar_in_binary();
}
