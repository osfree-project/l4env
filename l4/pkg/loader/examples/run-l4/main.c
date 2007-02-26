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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader-client.h>
#include <l4/env/errno.h>

static l4_threadid_t loader_id;
static l4_threadid_t fserv_id;

int
main(int argc, char **argv)
{
  int i, error;
  CORBA_Environment env = dice_default_environment;
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
	  "Start L4 application(s) by config script\n"
	  "Usage:\n"
	  "  %s <loader cfg_script> ... \n", fname);
      return -1;
    }

  /* we need the loader */
  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      fprintf(stderr, "LOADER not found\n");
      return -2;
    }

  /* we need a dataspace manager */
  if (!names_waitfor_name("FPROV-L4", &fserv_id, 3000))
    {
      fprintf(stderr, 
	      "FPROV-L4 not found.\n"
	      "Please start the fprov-l4 server first!\n");
      return -2;
    }

  printf("File provider is %x.%x\n", fserv_id.id.task, fserv_id.id.lthread);

  /* Parse command line: The loader has to load each config script
   * using fserv as file server. */
  for (i=1; i<argc; i++)
    {
      static char error_msg[1024];
      char *ptr = error_msg;
      if ((error = l4loader_app_open_call(&loader_id, argv[i], 
				          &fserv_id, 0, &ptr, &env)))
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

