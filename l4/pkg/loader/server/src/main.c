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
#include <l4/names/libnames.h>

#include <stdio.h>

#include "cfg.h"
#include "app.h"
#include "pager.h"
#include "idl.h"
#include "fprov-if.h"
#include "dm-if.h"
#include "exec-if.h"

char LOG_tag[9] = "loader";		     /**< tell logserver our log tag
						  before main is called */
const l4_ssize_t l4libc_heapsize = 128*1024; /**< init malloc heap */
const int l4thread_max_threads = 4;	     /**< limit number of threads */
const l4_size_t l4thread_stack_size = 16384; /**< limit stack size */

/** Main function. */
int
main(int argc, char **argv)
{
  int i, error;

  if (  (error = fprov_if_init())
      ||(error = exec_if_init())
      ||(error = app_init())
      ||(error = dm_if_init())
      ||(error = cfg_init())
      ||(error = start_app_pager()))
    return error;
  
  /* now we're ready to service ... */
  if (!names_register("LOADER"))
    {
      printf("failed to register LOADER\n");
      return -L4_EINVAL;
    }

  /* scan command line */
  if (argc < 2)
    printf("No config file (no command line?)\n");
  else
    {
      for (i=1; i<argc; i++)
	load_config_script(argv[i], tftp_id);
    }

  /* go into server mode */
  server_loop();

  return 0;
}

