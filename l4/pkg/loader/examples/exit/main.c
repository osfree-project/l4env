/**
 * \file	loader/examples/exit/main.c
 *
 * \author	Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 * \brief	Loads the L4 console program in a simple loop.
 *
 * 		The program depends on the L4 console.
 *
 * 		This programm is part of the test framework
 * 		for the events-exit-handling.
 */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4con/l4contxt.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/loader/loader-client.h>
#include <l4/env/env.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/exec/exec.h>
#include <l4/log/l4log.h>
#include <l4/events/events.h>
#include <l4/util/reboot.h>
#include <l4/util/rand.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char LOG_tag[9] = "exit";		/**< tell log library who we are */
l4_ssize_t l4libc_heapsize = -128*1024;
const int l4thread_max_threads = 6;

static l4_threadid_t loader_id;		/**< L4 thread id of L4 loader */
static l4_threadid_t tftp_id;		/**< L4 thread id of TFTP daemon */
static l4_threadid_t dm_id;		/**< L4 thread id of ds manager */


/** start an application */
static int
load_app(void)
{
  static char command[60] = "con_exit_demo";
  int error, i;
  CORBA_Environment env = dice_default_environment;
  static char error_msg[1024];
  char *ptr = error_msg;
  l4dm_dataspace_t dummy_ds = L4DM_INVALID_DATASPACE;
  l4_taskid_t task_ids[l4loader_MAX_TASK_ID];

  for (i=0; i<4; i++)
    {
      if ((error = l4loader_app_open_call(&loader_id, &dummy_ds, command,
					  &tftp_id, 0, task_ids, &ptr, &env)))
	{
	  printf("  Error %d (%s) loading application\n", 
	      error, l4env_errstr(error));
	  if (*error_msg)
	    printf("  (Loader says:'%s')\n", error_msg);
	  return error;
	}
      printf("  successfully loaded.\n");
    }

  return 0;
}



/** main function */
int
main(int argc, char *argv[])
{
  int error, i;
  int l = 0;

  if ((error = contxt_init(4096, 200)))
    {
      printf("Error %d opening contxt lib\n", error);
      return error;
    }

  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      printf("LOADER not found");
      return -2;
    }

  if (!names_waitfor_name("TFTP", &tftp_id, 3000))
    {
      printf("TFTP not found");
      return -2;
    }

  dm_id = l4env_get_default_dsm();

  i = 10;
 
  l4util_srand(100);

  while (i > 0)
    {
      printf("#%i\n", ++l);
      load_app();
      l4_sleep(5000);
    }

  return 0;
}
