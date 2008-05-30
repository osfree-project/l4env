/* (c) 2008 Technische Universitaet Dresden
 * This file is part of TUDOS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/util.h>
#include <l4/env/env.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/fprov_ext-client.h>
#include <l4/dm_mem/dm_mem.h>

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
  int error;
  int flags = 0;
  l4_threadid_t tftp_id;
  l4_threadid_t dm_id;
  l4dm_dataspace_t ds;
  void *addr;
  l4_size_t size = 4096;
  CORBA_Environment _env = dice_default_environment;
  
  if (!names_waitfor_name("TFTP", &tftp_id, 40000))
    {
      printf("TFTP not found\n");
      return -1;
    }

  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    {
      printf("No dataspace manager found\n");
      return -1;
    }

  if (argc != 2)
    {
      printf("Give file to write as command line parameter\n");
      exit(1);
    }

  if (!(addr = l4dm_mem_ds_allocate_named(size, flags, argv[1], &ds)))
    {
      printf("Allocating dataspace of size %d failed\n", size);
      return -L4_ENOMEM;
    }

  memcpy(addr, "Hello World!", 12);
  size = 12;

  if ((error = l4rm_detach(addr)))
    {
      printf("Error %d attaching dataspace\n", error);
      return -L4_ENOMEM;
    }

  /* set dataspace owner to server */
  if ((error = l4dm_transfer(&ds, tftp_id)))
    {
      printf("Error transfering dataspace ownership: %s (%d)\n",
             l4env_errstr(error), error);
      l4dm_close(&ds);

      return -L4_EINVAL;
    }

  if ((error = l4fprov_file_ext_write_call(&tftp_id, argv[1],
                                       &ds, size, &_env)))
    {
      printf("Error opening file from tftp\n");
      return -1;
    }
  
  printf("File %s was written\n", argv[1]);

  return 0;
}
