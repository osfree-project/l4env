IMPLEMENTATION:

#include <cstdio>
#include <cstdlib>

void kdb_ke (const char *msg)
{
  if (msg)
    puts (msg);

  puts ("Press Enter to continue, x to exit: ");

  if (getchar() == 'x')                             
    exit (EXIT_SUCCESS);
}
