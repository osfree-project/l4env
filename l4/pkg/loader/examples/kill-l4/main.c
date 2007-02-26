/* $Id$ */
/**
 * \file	loader/linux/kill-l4/main.c
 * \brief	Linux program to start applications on the L4 loader server
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/util/util.h>

int
main(int argc, char **argv)
{
  int i, error;
  const char *fname = strrchr(argv[0], '/');
  struct stat buf;

  if (stat("/proc/l4", &buf))
    {
      fprintf(stderr, "This binary requires L4Linux!\n");
      exit(-1);
    }

  if (!fname)
    fname = argv[0];
  else
    fname++;

  if (argc < 2)
    {
      fprintf(stderr,
	  "Kill an L4 task which was loaded by the L4 loader\n"
	  "Usage:\n"
	  "  %s <# l4-server> ...\n",
	  fname);
      return -1;
    }

  l4_sleep(1000);

  /* parse command line: Kill each task */
  for (i=1; i<argc; i++)
    {
      l4_taskid_t taskid;
      l4_uint32_t taskno = strtol(argv[i], 0, 0);

      if ((error = l4ts_taskno_to_taskid(taskno, &taskid)))
	printf("Error %d requesting taskid for taskno #%02x\n",
	    error, taskno);
      else
	if ((error = l4ts_kill_task_recursive(taskid)))
	  {
	    printf("Error %d killing l4 task \"%s\"\n", error, argv[i]);
	    return error;
	  }
    }

  return 0;
}
