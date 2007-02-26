/*
 * Small utility to switch the debugging option in the ORe server.
 */

#include <l4/ore/ore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int debug = 1;

char LOG_tag[9] = "ore_debug";

l4_ssize_t l4libc_heapsize = 4 * 1024;

int main(int argc, char **argv)
{

  if (argc > 1)
    {
      argv++;
      debug = atol(*argv);
    }

  printf("Setting debug level to %d\n", debug);

  l4ore_debug(debug);

  return 0;
}
