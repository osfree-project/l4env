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
#include <l4/dm_phys/dm_phys.h>

typedef char l4_page_t[L4_PAGESIZE];

static l4_threadid_t dm_id;
static l4_page_t map_page __attribute__ ((aligned(L4_PAGESIZE)));
static char buffer[L4_PAGESIZE+1];

int
main(int argc, char **argv)
{
  int i, error, size, print;
  unsigned offset = 0;
  CORBA_Environment env = dice_default_environment;
  l4dm_dataspace_t ds;
  l4_snd_fpage_t snd_fpage;

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

  /* map in map_area and set last byte to zero terminating string */
  memset(map_page, 0, sizeof(map_page));

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
      
      error = if_l4dm_generic_dump_call(&dm_id, &tid,L4DM_SAME_TASK,
			       &ds,&env);
      if ((error < 0) || (env.major != CORBA_NO_EXCEPTION))
	{
	  fprintf(stderr, "Error dumping l4 task \"%s\" (error %d, exc %d)\n", 
		  argv[i],error,env.major);
	  exit(error);
	}

      error = if_l4dm_mem_size_call(&(ds.manager), ds.id, &size, &env);
      if ((error < 0) || (env.major != CORBA_NO_EXCEPTION))
	{
	  fprintf(stderr, 
		  "Error determining size of ds %d at ds_manager %x.%x "
		  "(error %d, exc %d)\n",
		  ds.id, ds.manager.id.task, ds.manager.id.lthread,
		  error,env.major);
	  exit(error);
	}

      while (size > 0)
	{
	  // unmap map area
	  l4_fpage_unmap(l4_fpage((l4_addr_t)map_page, L4_LOG2_PAGESIZE,
				   L4_FPAGE_RW, L4_FPAGE_MAP),
			 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);

	  // page fault
	  error = 
	    if_l4dm_generic_fault_call(&(ds.manager),
			      ds.id,offset,&snd_fpage, &env);
	  if ((error < 0) || (env.major != CORBA_NO_EXCEPTION))
	    {
	      printf("Error requesting ds %d at ds_manager %x.%x "
		     "(error %d, exc %d)\n",
		     ds.id, ds.manager.id.task, ds.manager.id.lthread,
		     error,env.major);
	      if_l4dm_generic_close_call(&(ds.manager), ds.id, &env);
	      return error;
	    }

	  offset += L4_PAGESIZE;
	  
	  print = size;
	  if (print > L4_PAGESIZE)
	    print = L4_PAGESIZE;
	  
	  memcpy(buffer, map_page, L4_PAGESIZE);
	  buffer[L4_PAGESIZE]='\0';
	  
	  printf("%s", buffer);
	  
	  size -= print;
	}
      
      if_l4dm_generic_close_call(&(ds.manager), ds.id, &env);
    }

  return 0;
}

