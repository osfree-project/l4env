/**
 * \file        generic_fprov/examples/l4lx_get/main.c
 * \brief       Client for the fprov interface for L4Linux.
 *
 * \date        05/2006
 * \author      Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * Note, we assume to get small files here, if you start getting large files
 * you should probably extend the copy routine below to memcpy small chunks
 * in a loop (and thus reduce the size to malloc to one chunk).
 **/

/* (c) 2006 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */


#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <l4/sys/ipc.h>

#include <l4/names/libnames.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/generic_fprov/generic_fprov-client.h>
#include <l4/dm_phys/consts.h>

/* copy from l4/pkg/l4env/lib/include/__config.h */
#define L4ENV_DEFAULT_DSM_NAME     L4DM_MEMPHYS_NAME

int main(int argc, char **argv)
{
  const char *fprov_name, *path;
  l4_threadid_t fprov_id, dsm_id;
  CORBA_Environment _env = dice_default_environment;
  l4_size_t size;
  int err, ret = 0, fd;
  l4dm_dataspace_t ds;
  void *buf, *buf_linux;
  char *filename, *s;

  if (argc < 3)
    {
      fprintf(stderr, "Usage: %s FPROV-name path\n", *argv);
      return 1;
    }

  fprov_name = argv[1];
  path       = argv[2];

  if (!names_waitfor_name(fprov_name, &fprov_id, 2000))
    {
      fprintf(stderr, "Fprov server \"%s\" not found!\n", fprov_name);
      return 1;
    }


  if (!names_waitfor_name(L4ENV_DEFAULT_DSM_NAME, &dsm_id, 2000))
    {
      fprintf(stderr, "No dataspace manager found\n");
      return 1;
    }

  if ((err = l4fprov_file_open_call(&fprov_id, path, &dsm_id, 0,
                                    &ds, &size, &_env)))
    {
      printf("Opening file \"%s\" at %s failed: %s\n",
             path, fprov_name, l4env_errstr(err));
      return 1;
    }

  if ((err = l4rm_attach(&ds, size, 0, L4DM_RO | L4RM_MAP, &buf)))
    {
      fprintf(stderr, "Error attaching to DS\n");
      ret = 1;
      goto dm_close_out;
    }

  /* Now copy the buffer to some Linux known memory */
  if (!(buf_linux = malloc(size)))
    {
      fprintf(stderr, "Cannot malloc %d bytes\n", size);
      ret = 1;
      goto dm_close_out;
    }

  memcpy(buf_linux, buf, size);

  s = filename = (char *)path;
  while (*s)
    {
      if (*s == '/')
        filename = s + 1;
      s++;
    }

  if ((fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0666)) == -1)
    {
      perror("open");
      ret = 1;
      goto dm_close_out;
    }

  if (write(fd, buf_linux, size) != size)
    fprintf(stderr, "Error writing %d bytes to %s\n", size, filename);

  close(fd);

  free(buf_linux);


  if ((err = l4rm_detach(buf)))
    fprintf(stderr, "Error detaching from fprov region: %s\n",
                    l4env_errstr(err));

dm_close_out:
  if ((err = l4dm_close(&ds)))
    fprintf(stderr, "Error closing dataspace: %s\n", l4env_errstr(err));

  return ret;
}
