/* $Id$ */
/**
 * \file	loader/server/src/main.c
 * \brief	Main function
 * 
 * \date	08/29/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/util/parse_cmd.h>

#include <stdio.h>

#include "cfg.h"
#include "app.h"
#include "pager.h"
#include "idl.h"
#include "fprov-if.h"
#include "dm-if.h"
#include "events.h"

char LOG_tag[9] = "loader";		     /**< tell logserver our log tag
						  before main is called */
const l4_ssize_t l4libc_heapsize = 128*1024; /**< init malloc heap */
const int l4thread_max_threads = 5;	     /**< limit number of threads */
const l4_size_t l4thread_stack_size = 16384; /**< limit stack size */

int use_events;
int use_l4io;

/** Main function. */
int
main(int argc, const char **argv)
{
  int i, error;
  const char *fprov_name;
  
  if ((error = parse_cmdline(&argc, &argv,
		'f', "fprov", "specify file provider",
		PARSE_CMD_STRING, "TFTP", &fprov_name,
		'e', "events", "enable exit handling via events",
		PARSE_CMD_SWITCH, 1, &use_events,
		'i', "l4io", "use L4 I/O server for PCI management",
		PARSE_CMD_SWITCH, 1, &use_l4io,
		0)))
    {
      switch (error)
	{
	case -1: printf("Bad parameter for parse_cmdline()\n"); break;
	case -2: printf("Out of memory in parse_cmdline()\n"); break;
	default: printf("Error %d in parse_cmdline()\n", error); break;
	}
      return 1;
    }
  if (  
#ifndef USE_LDSO
        (error = exec_if_init()) ||
#endif
        (error = dm_if_init())
      ||(error = cfg_init())
      ||(error = start_app_pager()))
    return error;
  
  /* start thread listening for exit events */
  if (use_events) 
    init_events();

  /* now we're ready to service ... */
  if (!names_register("LOADER"))
    {
      printf("failed to register LOADER\n");
      return -L4_EINVAL;
    }

  /* scan command line */
  if (argc < 1)
    printf("No config file (no command line?)\n");

  else
    {
      l4_threadid_t fprov_id;
      l4_threadid_t myself_id = l4_myself();

      /* file provider */
      if (!names_waitfor_name(fprov_name, &fprov_id, 30000))
	{
	  printf("File provider \"%s\" not found\n", fprov_name);
	  return -L4_ENOTFOUND;
	}

      for (i=0; i<argc; i++)
	if (argv[i])
	  load_config_script_from_file(argv[i], fprov_id, myself_id, 0, 0);
    }

  /* go into server mode */
  server_loop();

  return 0;
}
