/*
 * \brief   Ext2fs specific plugin for file-I/O
 * \date    2004-03-17
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/* standard includes */
#include <stdio.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/ext2fs/ext2fs.h>
#include <l4/ext2fs/ext2fs-simple-client.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/atomic.h>

/* local */
#include "io_ext2fs.h"

/*****************************************************************************
 *** globals
 *****************************************************************************/

/* ext2fs client initialized */
static int ext2fs_initialized = 0;

/* ext2fs file server thread id */
static l4_threadid_t ext2fs_id = L4_INVALID_ID;

/* debug config */
#define DEBUG_INIT    1
#define DEBUG_OPEN    1
#define DEBUG_CLOSE   1
#define DEBUG_READ    0
#define DEBUG_LSEEK   0

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Initialize ext2fs client
 */
/*****************************************************************************/
static int
__init_ext2fs (void)
{
  l4_threadid_t fs_id;
  CORBA_Environment _env = dice_default_environment;
  int ret;

  /* request ext2fs server at namesserver */
  if (!names_waitfor_name (L4EXT2_NAMES, &fs_id, 30000))
  {
    LOG_Error ("ext2fs server not found");
    return -1;
  }

  LOGdL (DEBUG_INIT, "ext2fs server at " l4util_idfmt, l4util_idstr (fs_id));

  /* open new client context */
  ret =
    l4ext2fs_fs_open_call (&fs_id, "0x00", L4EXT2_RDWR, &ext2fs_id, &_env);
  if ((ret < 0) || (_env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error ("open file server failed: %s (%d)", l4env_errstr (ret), ret);
  }

  LOGdL (DEBUG_INIT, "ext2fs service at " l4util_idfmt,
	 l4util_idstr (ext2fs_id));

  return 0;
}


/*
 * open
 */
int
io_ext2fs_open (const char *__name, int __mode, ...)
{
  int fd;
  CORBA_Environment _env = dice_default_environment;

  if (l4util_cmpxchg32 (&ext2fs_initialized, 0, 1))
  {
    if (__init_ext2fs () < 0)
      return -1;
  }

  LOGdL (DEBUG_OPEN, "open file \'%s\'", __name);

  fd = l4ext2fs_file_open_call (&ext2fs_id, __name, L4EXT2_RDONLY, 0, &_env);
  if (fd < 0)
  {
    LOG_Error ("open file  \'%s\' failed: %s (%d)",
	       __name, l4env_errstr (fd), fd);
    return -1;
  }

  LOGd (DEBUG_OPEN, "opened file, fd %d", fd);

  /* done */
  return fd;
}


/*
 * close
 */
int
io_ext2fs_close (int __fd)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  LOGdL (DEBUG_CLOSE, "fd %d", __fd);

  ret = l4ext2fs_file_close_call (&ext2fs_id, __fd, &_env);
  if ((ret < 0) || (_env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error ("l4ext2fs_file_close_call failed: %s (%d, exc %d)",
	       l4env_errstr (ret), ret, _env.major);
    return -1;
  }

  /* done */
  return 0;
}


/*
 * read
 */
unsigned long
io_ext2fs_read (int __fd, void *__buf, unsigned long __n)
{
  int ret;
  CORBA_Environment _env = dice_default_environment;

  LOGdL (DEBUG_READ, "fd %d, %lu bytes, buffer at 0x%08x", __fd, __n,
	 (unsigned) __buf);

  ret = l4ext2fs_file_read_call (&ext2fs_id, __fd, (l4_uint8_t **) & __buf,
				 (l4_uint32_t *) & __n, &_env);
  if ((ret < 0) || (_env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error ("l4ext2fs_file_read_call() failed: %s (%d, exc %d)",
	       l4env_errstr (ret), ret, _env.major);
    return -1;
  }

  /* done */
  return __n;
}


/*
 * seek
 */
long
io_ext2fs_lseek (int __fd, long __offset, int __whence)
{
  CORBA_Environment _env = dice_default_environment;
  int whence;
  unsigned long new_pos;

  LOGdL (DEBUG_LSEEK, "fd %d, offset %lu, whence %d", __fd, __offset,
	 __whence);

  switch (__whence)
  {
  case SEEK_SET:
    whence = L4EXT2_SEEK_SET;
    break;
  case SEEK_CUR:
    whence = L4EXT2_SEEK_CUR;
    break;
  case SEEK_END:
    whence = L4EXT2_SEEK_END;
    break;
  default:
    LOG_Error ("invalid seek direction: %d", __whence);
    return -1;
  }

  new_pos =
    l4ext2fs_file_lseek_call (&ext2fs_id, __fd, __offset, whence, &_env);
  if ((new_pos == -1) || (_env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error ("l4ext2fs_file_lseek_call() failed (ret %d, exc %d)",
	       (int) new_pos, _env.major);
    return -1;
  }

  /* done */
  return new_pos;
}



/*
 * not implemented yet 
 */

unsigned long
io_ext2fs_write (int __fd, void *__buf, unsigned long __n)
{
  KDEBUG ("ext2fs_write");
  /* done */
  return 0;
}


unsigned long
io_ext2fs_fread (void *__buf, unsigned long __fact, unsigned long __n,
		 int __fd)
{
  KDEBUG ("ext2fs_fread");
  /* done */
  return 0;
}


int
io_ext2fs_ftruncate (int __fd, unsigned long __offset)
{
  KDEBUG ("ext2fs_ftruncate");
  /* done */
  return 0;
}


int
io_ext2fs_fstat (int __fd, struct stat *buf)
{
  KDEBUG ("ext2fs_fstat");
  /* done */
  return 0;
}
