
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>

#include <stdio.h>

int
main(void)
{
  int i;

  for (i=0; i<10; i++)
    {
      l4_threadid_t myself = l4_myself();
      
      printf("Hello World, I am %x.%x!\n", 
	  myself.id.task, myself.id.lthread);

      l4_sleep(2000);
    }

  return 0;
}

