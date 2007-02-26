/* $Id$ */
/**
 * \file	loader/linux/run-l4/main.c
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Linux program to start applications on the L4 loader server */

#include <stdio.h>
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
  sm_exc_t exc;
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
      if ((error = l4loader_app_open(loader_id, argv[i], 
				     (l4loader_threadid_t*)&fserv_id, 0, &exc)))
	{
	  printf("Error %d (%s) loading l4 module \"%s\"\n", 
	      error, l4env_errstr(error), argv[i]);
	  return error;
	}
    }

  return 0;
}

