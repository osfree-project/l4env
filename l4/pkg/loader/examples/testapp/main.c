
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/kdebug.h>

#include <stdio.h>

char LOG_tag[9] = "test";		/**< tell log library who we are */

int
main(int argc, char *argv[])
{
  int i;

  printf("\nHello World!\n\n");

  printf("ARGC=%d\n", argc);
  for (i=0; i<argc; i++)
    printf("  ARGV[0] = %s\n", argv[i]);
  printf("\n");

  return 0;
}

