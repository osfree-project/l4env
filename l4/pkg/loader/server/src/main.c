/* $Id$ */
/**
 * \file	loader/server/src/main.c
 * 
 * \date	08/29/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * 
 * \brief	Main function */

#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/env/env.h>
#include <l4/oskit10_l4env/support.h>

#include <stdio.h>

#include "cfg.h"
#include "app.h"
#include "pager.h"
#include "idl.h"
#include "fprov-if.h"
#include "dm-if.h"

char LOG_tag[9] = "loader";		/**< tell logserver our log tag
					  before main is called */

const int l4thread_max_threads = 4;
const l4_size_t l4thread_stack_size = 16384;

/** Main function. */
int
main(int argc, char **argv)
{
  int i, error;

  if ((  error = OSKit_libc_support_init(0x20000))
      ||(error = fprov_if_init())
      ||(error = app_init())
      ||(error = dm_if_init())
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

