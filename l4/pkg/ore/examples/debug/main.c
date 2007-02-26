/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

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
  l4ore_config c = L4ORE_DEFAULT_CONFIG;
  strncpy(c.ro_orename, "ORe", 3);

  if (argc > 1)
    {
      argv++;
      debug = atol(*argv);
    }

  printf("Setting debug level to %d\n", debug);

  l4ore_debug(&c, debug);

  return 0;
}
