/* $Id$ */
/**
 * \file	dm_phys/examples/list-ds/main.c
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Linux program to list memory regions of DMphys */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/env/env.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/dm_phys/dm_phys.h>

static l4_threadid_t dm_id;

int
main(int argc, char **argv)
{
  int i;
  CORBA_Environment env = dice_default_environment;

  if (argc < 2)
    {
      fprintf(stderr, 
	  "Request information from dataspace manager about dataspaces\n"
	  "and list it to L4 debug console\n"
	  "  Usage:\n"
	  "  %s <l4-task-id> ... list information owned by specific L4 task\n" 
	  "  %s 0            ... list information about all L4 tasks\n",
	  argv[0], argv[0]);
      return -1;
    }

  /* we need the simple_ds server */
  if (!names_waitfor_name(L4DM_MEMPHYS_NAME, &dm_id, 3000))
    {
      fprintf(stderr, "DMphys not found\n");
      return -2;
    }

  /* parse command line: Kill each task */
  for (i=1; i<argc; i++)
    {
      int task_id = strtol(argv[i], 0, 0);
      l4_threadid_t tid;
      
      if (task_id == 0)
	tid = L4_INVALID_ID;
      else
	{
	  tid.id.task    = task_id;
	  tid.id.lthread = 0;
	}

      if_l4dm_generic_list_call(&dm_id, &tid, L4DM_SAME_TASK, &env);
      if (env.major != CORBA_NO_EXCEPTION)
	{
	  printf("Error listing l4 task \"%s\" (exc %d)\n",argv[i],env.major);
	  exit(-1);
	}
    }

  return 0;
}

