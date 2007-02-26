
#include <l4/sys/kdebug.h>

#include <stdio.h>

int
main(void)
{
  printf("\nHello World!\n\n");

  enter_kdebug("App Done");
  return 0;
}

