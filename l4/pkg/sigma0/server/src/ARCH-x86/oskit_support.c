#include <stdio.h>
#include <unistd.h>

#include <flux/machine/pc/direct_cons.h>
#include <flux/machine/pc/reset.h>

#include <l4/sys/kdebug.h>

#include "globals.h"

int
putchar(int c)
{
  if (!quiet)
    {
      outchar(c);
      if (c=='\n')
	outchar('\r');
    }
  return c;
}

int
getchar(void)
{
  return direct_cons_getchar();
}

void 
_exit(int fd)
{
  char c;

  printf("\nReturn reboots, \"k\" enters L4 kernel debugger...\n");

  c = getchar();

  if (c == 'k' || c == 'K') 
    enter_kdebug("before reboot");

  pc_reset();
}
