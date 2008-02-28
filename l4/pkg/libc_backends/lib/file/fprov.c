/*!
 * \file   fprov.c
 * \brief  open/read/seek/close implementation for fprov interface
 *
 * \date   2008-02-27
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
/*
 * NOTE: THIS IS NOT THREAD SAFE!
 * NOTE2: this implementation is not complete, if you (yes, you!) need more,
 * you (yes, you!, again) can extend it
 */

#include "libfile.h"

#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/env.h>
#include <l4/generic_fprov/generic_fprov-client.h>
#include <stdio.h>
#include <errno.h>

enum { fd_offset = 3 };
static l4_threadid_t fprov_id;
static const char *fprov_name = "BMODFS";
static l4_threadid_t dm_id;

struct struct_slot {
  l4dm_dataspace_t ds;
  void *addr;
  int pos;
  l4_size_t size;
};

static struct struct_slot slot[1];

static int connect_fprov(void)
{
  if (!l4_is_nil_id(fprov_id) && !l4_is_nil_id(dm_id))
    return 1;

  if (!names_waitfor_name(fprov_name, &fprov_id, 10000))
    {
      printf("Could not find \"%s\"\n", fprov_name);
      return 0;
    }

  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    {
      printf("No dataspace manager found\n");
      return 0;
    }

  return 1;
}

static int find_free(void)
{
  int i = 0;
  for (; i < sizeof(slot) / sizeof(slot[0]); ++i)
    if (!slot[i].addr)
      return i;
  return -1;
}

static void close_dm(int fd)
{
  int err;
  if ((err = l4dm_close(&slot[fd].ds)))
    printf("l4dm_close(): %s", l4env_errstr(err));
}

int open64(const char *name, int flags, int mode)
{
  int err;
  DICE_DECLARE_ENV(_env);
  int fd;

  if (!connect_fprov())
    return -1;

  if (-1 == (fd = find_free()))
    return -1;

  slot[fd].pos = 0;

  err = l4fprov_file_open_call(&fprov_id, name, &dm_id, 0,
                               &slot[fd].ds, &slot[fd].size, &_env);

  if (err)
    {
      printf("Error opening file \"%s\" at %s: %s\n",
             name, fprov_name, l4env_errstr(err));
      return -1;
    }

  if ((err = l4rm_attach(&slot[fd].ds, slot[fd].size, 0, L4DM_RO | L4RM_MAP,
                         &slot[fd].addr)))
    {
      printf("Error attaching dataspace: %s\n", l4env_errstr(err));
      close_dm(fd);
      return -1;
    }

  return fd + fd_offset;
}

int open(const char *name, int flags, int mode)
{
  return open64(name, flags, mode);
}

static int check_fd(int fd)
{
  return fd < fd_offset || fd >= (sizeof(slot) / sizeof(slot[0]) + fd_offset);
}

size_t read(int fd, void *buf, size_t count)
{
  if (check_fd(fd))
    {
      errno = EBADF;
      return -1;
    }
  fd -= fd_offset;

  if (slot[fd].pos + count > slot[fd].size)
    count = slot[fd].size - slot[fd].pos;

  if (count <= 0)
    return -1;

  memcpy(buf, (char *)slot[fd].addr + slot[fd].pos, count);
  slot[fd].pos += count;

  return count;
}

unsigned int lseek(int fd, unsigned int offset, int whence)
{
  if (check_fd(fd))
    {
      errno = EBADF;
      return -1;
    }
  fd -= fd_offset;

  switch (whence)
    {
      case SEEK_SET: slot[fd].pos = offset; break;
      case SEEK_CUR: slot[fd].pos += offset; break;
      case SEEK_END: slot[fd].pos = slot[fd].size + offset; break;
      default: errno = EINVAL; return -1;
    };

  if (slot[fd].pos < 0)
    {
      errno = EINVAL;
      return -1;
    }

  return slot[fd].pos;
}

int close(int fd)
{
  int err;
  if (check_fd(fd))
    {
      errno = EBADF;
      return -1;
    }
  fd -= fd_offset;
  if ((err = l4rm_detach(slot[fd].addr)))
    {
      printf("l4rm_detach(): %s", l4env_errstr(err));
      return -1;
    }

  close_dm(fd);
  return 0;
}
