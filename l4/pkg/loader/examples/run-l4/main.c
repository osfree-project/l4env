/* $Id$ */
/**
 * \file	loader/linux/run-l4/main.c
 * \brief	Linux program to start applications on the L4 loader server
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader-client.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>

static l4_threadid_t loader_id;
static l4_threadid_t fserv_id;

int
main(int argc, const char *argv[])
{
  int i, error;
  DICE_DECLARE_ENV(env);
  const char *fname = strrchr(argv[0], '/');
  struct stat buf;
  const char *fprov_name;

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
              "Start L4 application(s) via the loader\n"
              "Usage:\n"
              "  %s {program,loader_config_script} ... \n", fname);
      return -1;
    }

  if ((error = parse_cmdline(&argc, &argv,
		'f', "fprov", "specify file provider",
		PARSE_CMD_STRING, "FPROV-L4", &fprov_name,
		0)))
    {
      switch (error)
	{
	case -1: printf("Bad parameter for parse_cmdline()"); break;
	case -2: printf("Out of memory in parse_cmdline()"); break;
	case -3:
	case -4: return 1;
	default: printf("Error %d in parse_cmdline()", error); break;
	}
      return 1;
    }

  /* we need the loader */
  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      fprintf(stderr, "LOADER not found\n");
      return -2;
    }

  /* we need a file provider */
  if (!names_waitfor_name(fprov_name, &fserv_id, 3000))
    {
      fprintf(stderr,
	      "%s not found.\n"
	      "Please start a file provider server first!\n", fprov_name);
      return -2;
    }

  printf("File provider is " l4util_idfmt "\n", l4util_idstr(fserv_id));

  /* Parse command line: The loader has to load each config script
   * using fserv as file server. */
  for (i = 0; i < argc; i++)
    {
      static char error_msg[1024];
      char *ptr = error_msg;
      char rp[PATH_MAX];
      const char *rp2;
      static l4_taskid_t task_ids[l4loader_MAX_TASK_ID];
      l4dm_dataspace_t dummy_ds = L4DM_INVALID_DATASPACE;
      char *space_char;
  
      /* get the absolute pathname */
      if ((space_char = strchr(argv[i], ' ')))
	*space_char = '\0';
      rp2 = realpath(argv[i], rp);
      if (rp2 == NULL)
	/* realpath not found */
	rp2 = argv[i];

      /* Append argument if available */
      if (space_char)
        {
	  *space_char = ' ';
	  strncat(rp, space_char, sizeof(rp) - strlen(rp) - 1);
        }

      if ((error = l4loader_app_open_call(&loader_id, &dummy_ds, rp2,
				          &fserv_id, 0, task_ids,
					  &ptr, &env)))
	{
	  printf("Error %d loading l4 module \"%s\"\n",
	         error, argv[i]);
	  if (*error_msg)
	    printf("(Loader says '%s')\n", error_msg);
	  return error;
	}
    }
  return 0;
}
