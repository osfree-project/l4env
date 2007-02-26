
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/util.h>
#include <l4/env/env.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-client.h>

#include <stdio.h>

int
main(int argc, char **argv)
{
  int error;
  l4_threadid_t tftp_id;
  l4_threadid_t dm_id;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_size_t size;
  sm_exc_t exc;
  
  LOG_init("tftptst");
  
  if (!names_waitfor_name("TFTP", &tftp_id, 10000))
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
      printf("Give file to open as command line parameter\n");
      exit(1);
    }

  if ((error = l4fprov_file_open(tftp_id,
			         argv[1],
				 (l4fprov_threadid_t*)&dm_id,
				 0, (l4fprov_dataspace_t*)&ds, &size, &exc)))
    {
      printf("Error opening file from tftp\n");
      return -1;
    }
  
  if ((error = l4rm_attach(&ds, size, 0, L4DM_RO | L4RM_MAP, (void **)&addr)))
    {
      printf("Error %d attaching dataspace\n", error);
      return -L4_ENOMEM;
    }

  printf("File %s opened at %08x.\n", argv[1], addr);

  return 1;
}

