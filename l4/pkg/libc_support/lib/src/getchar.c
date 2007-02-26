#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

#include <l4/env_support/getchar.h>

int getchar(void)
{
  int ch;

  while ((ch = l4kd_inchar()) == -1)
    {
      l4_sleep(50);
    }

  return ch;
}
