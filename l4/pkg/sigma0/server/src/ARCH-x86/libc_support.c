#include <stdio.h>
#include <unistd.h>

#include <l4/sys/kdebug.h>
#include <l4/env_support/getchar.h>
#include <l4/util/util.h>

#include "globals.h"

void __attribute__((noreturn)) l4util_reboot_arch(void);

void 
_exit(int fd)
{
  char c;

  printf("\nReturn reboots, \"k\" enters L4 kernel debugger...\n");

  c = getchar();

  if (c == 'k' || c == 'K') 
    enter_kdebug("before reboot");

  l4util_reboot_arch();
}

void __thread_doexit(int code);
void __thread_doexit(int code)
{
}

void abort(void);
void abort(void)
{
  exit(1);
}
