/* $Id$ */
/**
 * \file	dm_phys/examples/dump-ds/main.c
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Linux program to dump memory regions of DMphys */

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
#include <l4/env/errno.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/util/l4_macros.h>

int
main(int argc, char **argv)
{
  int i;
  l4_threadid_t dm_id;
  l4dm_dataspace_t ds, next_ds;

  if (argc < 2)
    {
      fprintf(stderr, 
	  "Request information from dataspace manager about dataspaces\n"
	  "and dump it to the current console\n"
	  "  Usage:\n"
	  "  %s <l4-task-id> ... dump information owned by specific L4 task\n" 
	  "  %s 0            ... dump information about all L4 tasks\n",
	  argv[0], argv[0]);
      return -1;
    }

  /* we need the simple_ds server */
  if (!names_waitfor_name(L4DM_MEMPHYS_NAME, &dm_id, 3000))
    {
      fprintf(stderr, "DMphys not found\n");
      return -2;
    }

  /* parse command line: dump each task */
  for (i=1; i<argc; i++)
    {
      int task_id = strtol(argv[i], 0, 0);
      l4_threadid_t tid;
      l4_threadid_t owner;
      l4_size_t size, total_size;
      char name[L4DM_DS_NAME_MAX_LEN+1];

      if (task_id == 0)
	tid = L4_INVALID_ID;
      else
	{
	  tid.id.task    = task_id;
	  tid.id.lthread = 0;
	}

restart:
      ds         = L4DM_INVALID_DATASPACE;
      ds.manager = dm_id;

      /* retrieve the first dataspace id */
      if (l4dm_mem_info(&ds, &size, &owner, name, &ds.id) == -L4_ENOTFOUND)
	exit(-1);

      for (total_size = 0; 
	   ds.id != L4DM_INVALID_DATASPACE.id; ds.id = next_ds.id)
	{
	  if (l4dm_mem_info(&ds, &size, &owner, name, &next_ds.id) 
	        == -L4_ENOTFOUND)
	    goto restart;
	  if (!l4_is_invalid_id(tid) && !l4_tasknum_equal(owner, tid))
	    continue;
	  printf("%5u: size=%08x (%7uKB,%4uMB) owner="
	      l4util_idfmt_adjust" name=\"%s\"\n",
	      ds.id, size, (size+(1<<9)-1)/(1<<10), (size+(1<<19)-1)/(1<<20),
	      l4util_idstr(owner), name);
	  total_size += size;
	}

      if (total_size)
	{
	  printf("==========================================================="
	      "========================\n"
	      " total size %08x (%7uKB,%4uMB)\n",
	      total_size, (total_size+(1<<9)-1)/(1<<10), 
	      (size+(1<<19)-1)/(1<<20));
	}
    }

  return 0;
}
