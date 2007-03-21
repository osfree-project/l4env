/* $Id$ */
/**
 * \file	loader/linux/dump-l4/main.c
 * \brief	L4Linux program to dump information about an application
 *		which was started by the L4 loader.
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
#include <l4/sys/syscalls.h>
#include <l4/util/l4_macros.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader-client.h>
#include <l4/env/env.h>
#include <l4/dm_mem/dm_mem.h>

typedef char l4_page_t[L4_PAGESIZE];

static l4_threadid_t loader_id;

/** make sure that map_page is L4 pagesize aligned */
static l4_page_t map_page __attribute__ ((aligned(L4_PAGESIZE)));
static l4_page_t task_list;

/** buffer for filename */
static char fname_buf[1024];

/** dump L4 environment inforpage */
static void
dump_l4env_infopage(l4env_infopage_t *env)
{
  int i, j, k, pos=0;

  static char *type_names_on[] =
    {"r", "w", "x", " reloc", " link", " page", " reserve",
      " share", " beg", " end", " errlink" };
  static char *type_names_off[] =
    {"-", "-", "-", "", "", "", "", "", "", "", "" };

  for (i=0; i<env->section_num; i++)
    {
      l4exec_section_t *l4exc = env->section + i;
      char pos_str[8]="   ";

      if (l4exc->info.type & L4_DSTYPE_OBJ_BEGIN)
	{
	  sprintf(pos_str, "%3d", pos);
	  pos++;
	}

      printf("  %s ds %3d: %08lx-%08lx ",
	   pos_str, l4exc->ds.id, l4exc->addr, l4exc->addr+l4exc->size);
      for (j=1, k=0; j<=L4_DSTYPE_ERRLINK; j<<=1, k++)
	{
	  if (l4exc->info.type & j)
	    printf("%s",type_names_on[k]);
	  else
	    printf("%s",type_names_off[k]);
	}
      printf("\n");
    }
}

/** dump task information */
static int
dump_task_info(unsigned int task)
{
  int error;
  char *fname = fname_buf;
  l4dm_dataspace_t ds;
  l4_addr_t fpage_addr;
  l4_size_t fpage_size;
  DICE_DECLARE_ENV(env);

  if ((error = l4loader_app_info_call(&loader_id, task, 0, &fname,
				      &ds, &env)))
    {
      printf("Error %d dumping l4 task %x\n", error, task);
      return error;
    }

  printf("Task \"%s\", #%x\n", fname, task);

  /* unmap memory */
  l4_fpage_unmap(l4_fpage((l4_umword_t)map_page, L4_LOG2_PAGESIZE,
			  L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);

  /* page in L4 page */
  error = l4dm_map_pages(&ds,0,L4_PAGESIZE,(l4_addr_t)map_page,
			 L4_LOG2_PAGESIZE,0,L4DM_RO,&fpage_addr,&fpage_size);
  if (error < 0)
    {
      printf("Error %d requesting ds %d at ds_manager "l4util_idfmt"\n",
	  error, ds.id, l4util_idstr(ds.manager));
      l4dm_close(&ds);
      return error;
    }

  /* dump information we need */
  dump_l4env_infopage((l4env_infopage_t*)map_page);

  /* junk dataspace */
  l4dm_close(&ds);

  return 0;
}

/** the main function */
int
main(int argc, char **argv)
{
  int i, error;
  struct stat buf;

  if (stat("/proc/l4", &buf))
    {
      fprintf(stderr, "This binary requires L4Linux!\n");
      exit(-1);
    }

  if (argc < 2)
    {
      const char *fname = strrchr(argv[0], '/');

      if (!fname)
	fname = argv[0];
      else
	fname++;

      fprintf(stderr,
	  "Dump information of an L4 task which was loaded by the L4 loader\n"
	  "Usage:\n"
	  "  %s <l4-server> ... dump information about a specific L4 task\n"
	  "  %s 0           ... dump information about all L4 tasks\n",
	  fname, fname);
      return -1;
    }

  /* we need the loader */
  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      fprintf(stderr, "LOADER not found\n");
      return -2;
    }

  /* map in map_area */
  memset(map_page, 0, sizeof(map_page));

  /* parse command line: dump each task */
  for (i=1; i<argc; i++)
    {
      int task = strtol(argv[i], 0, 0);

      if (task == 0)
	{
	  char *fname = fname_buf;
	  unsigned int *ptr;
	  l4dm_dataspace_t ds;
	  l4_addr_t fpage_addr;
	  l4_size_t fpage_size;
	  CORBA_Environment env = dice_default_environment;

	  if ((error = l4loader_app_info_call(&loader_id, 0, 0, &fname,
					      &ds, &env)))
	    {
	      printf("Error %d getting l4 task list\n", error);
	      return error;
	    }

	  /* unmap memory */
	  l4_fpage_unmap(l4_fpage((l4_umword_t)map_page, L4_LOG2_PAGESIZE,
			           L4_FPAGE_RW, L4_FPAGE_MAP),
			 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);

	  /* page in L4 page */
	  error = l4dm_map_pages(&ds,0,L4_PAGESIZE,(l4_addr_t)map_page,
				 L4_LOG2_PAGESIZE,0,L4DM_RO,
				 &fpage_addr,&fpage_size);
	  if (error < 0)
	    {
	      printf("Error %d requesting ds %d at ds_manager "l4util_idfmt"\n",
		  error, ds.id, l4util_idstr(ds.manager));
	      l4dm_close(&ds);
	      return error;
	    }

	  memcpy(task_list, map_page, L4_PAGESIZE);

	  for (ptr=(unsigned int*)task_list; *ptr != 0; ptr++)
	    {
	      dump_task_info(*ptr);
	    }
	}
      else
	{
	  return dump_task_info(task);
	}
    }

  return 0;
}

