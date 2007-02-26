/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/examples/oskit/main.c
 * \brief  Block device driver based on the OSKit
 *
 * \date   09/07/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/getopt.h>
#include <l4/names/libnames.h>

/* private includes */
#include "blksrv.h"

/* global stuff */
char LOG_tag[9]="blkoskit";

const int l4thread_max_threads = 64;

/*****************************************************************************/
/**
 * \brief Parse command line
 */
/*****************************************************************************/ 
static void
__parse_cmdline(int argc, char * argv[])
{
  static struct option long_options[] =
    {
      {"write",  0, 0, 'w'},
      {"ide",    0, 0, 'i'},
      {"scsi",   1, 0, 's'},
      {"device", 1, 0, 'd'},
      {0, 0, 0, 0}      
    };
  char c;

  while(1)
    {
      c = getopt_long(argc, argv, "wis:d:", long_options, NULL);
      if (c == -1)
        break;

      switch (c)
        {
        case 'w':
          /* open device read-write */
          blksrv_dev_read_write();
          break;

        case 'i':
          /* use IDE driver */
          blksrv_dev_use_ide();
          break;
          
        case 's':
          /* use SCSI driver, the argument specifies the host adapter */
          blksrv_dev_use_scsi(optarg);
          break;

        case 'd':
          /* device */
          blksrv_dev_set_device(optarg);
          break;

        default:
          LOG_Error("invalid option \"%c\"", c);
        }
    }
}

/*****************************************************************************/
/**
 * \brief Main.
 */
/*****************************************************************************/ 
int 
main(int argc, char * argv[])
{
  int ret;

  /* parse command line */
  __parse_cmdline(argc,argv);
  
  /* start request thread, this also initializes the OSKit drivers */
  ret = blksrv_start_request_thread();
  if (ret < 0)
    {
      LOG_Error("start request service thread failed: %s (%d)",
                l4env_errstr(ret), ret);
      exit(1);
    }

  /* start command interface thread */
  ret = blksrv_start_command_thread();
  if (ret < 0)
    {
      LOG_Error("start command interface thread failed: %s (%d)",
                l4env_errstr(ret), ret);
      exit(1);
    }

  /* register at nameserver */
  if (!names_register(NAMES_OSKITBLK))
    {
      LOG_Error("register at nameserver failed!");
      exit(1);
    }

  /* start driver server loop */
  blksrv_start_driver();

  /* done */
  return 0;
}
