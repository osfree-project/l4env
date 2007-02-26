/*
 * \brief   grub loader specific plugin for file-I/O
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


/* stdlib */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* OSKit */
#include <l4/env/mb_info.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>
#include <l4/semaphore/semaphore.h>

/* local */
#include "io_grubfs.h"



/* maximum number of files via can access */
#define MAXFILES 32
/* maximum number of files open at once */
#define MAXFILES_OPEN 16

static l4_addr_t fops_file[MAXFILES][2];
/* number of files found */
static int fops_file_no = 0;

/* size of file as we got it from grub */
static long fops_file_size[MAXFILES];
/* filesize after writing */
static long fops_file_lenght[MAXFILES];
/* adress of file in mem */
unsigned char *fops_file_pointer[MAXFILES];


typedef struct fops_t
{
  /* internal fileno */
  int file_no;
  /* current position */
  unsigned long cur_index;
}
fops_t;

/* simple list of currently opened files */
/* NOTE: the returned fdes is an index for this array */
static fops_t file_list[MAXFILES_OPEN];

/* semaphore for accessing file list */
static l4semaphore_t file_semaphore = L4SEMAPHORE_UNLOCKED;
;

/* internal prototypes */
static int name2fileno (const char *__name);
static int io_grubfs_init_grub_mods (void);
static int io_grubfs_init (void);

/* macros for simplicity */
#define FD2NO    (file_list[__fd].file_no)
#define FD2INDEX (file_list[__fd].cur_index)


/*
 * open
 */
int
io_grubfs_open (const char *__name, int __mode, ...)
{
  int __fd;

  /* lock */
  l4semaphore_down (&file_semaphore);

  /* init if not already done. */
  if (fops_file_no == 0)
    io_grubfs_init ();

  /* find first free entry in list of opened files */
  for (__fd = 0; __fd < MAXFILES_OPEN; __fd++)
  {
    if (FD2NO == -1)
    {
      /* found a free entry */
      FD2INDEX = 0;
      FD2NO = name2fileno (__name);
      /* unlock */
      l4semaphore_up (&file_semaphore);
      /* check if fileno is valid */
      if ((FD2NO >= 0) && (FD2NO < MAXFILES))
	/* file found */
	return __fd;
      else
	/* file couldn't found */
	return -1;
    }
  }
  /* here no free entry found */
  printf ("Warning: maximum number of opened files reached!\n");

  /* unlock */
  l4semaphore_up (&file_semaphore);

  return -1;
}

/*
 * close
 */
int
io_grubfs_close (int __fd)
{
  /* invalid index */
  if (__fd >= MAXFILES_OPEN)
    return -1;
  /* lock */
  l4semaphore_down (&file_semaphore);
  /* set entry to free */
  FD2NO = -1;
  /* unlock */
  l4semaphore_up (&file_semaphore);

  /* ok */
  return 0;
}


/*
 * read
 */
unsigned long
io_grubfs_read (int __fd, void *__buf, unsigned long __n)
{
  unsigned long copy_size = 0;

  /* invalid index */
  if (__fd >= MAXFILES_OPEN)
    return -1;

  /* calculate size to copy */
  if ((FD2INDEX + __n) > fops_file_lenght[FD2NO])
    copy_size = fops_file_lenght[FD2NO] - FD2INDEX;
  else
    copy_size = __n;

  /* check it */
  if ((FD2INDEX > fops_file_lenght[FD2NO]) || (copy_size == 0))
    return -1;

  /* copy into user space */
  memcpy (__buf, fops_file_pointer[FD2NO] + FD2INDEX, copy_size);
  FD2INDEX = FD2INDEX + copy_size;

  return copy_size;
}


/*
 * write
 */
unsigned long
io_grubfs_write (int __fd, void *__buf, unsigned long __n)
{
  unsigned long copy_size = 0;

  /* invalid index */
  if (__fd >= MAXFILES_OPEN)
    return -1;

  if ((FD2INDEX + __n) > fops_file_size[__fd])
    copy_size = fops_file_size[FD2NO] - FD2INDEX;
  else
    copy_size = __n;

  /* still enought space? */
  if ((FD2INDEX > fops_file_size[FD2NO]) || (copy_size == 0))
    return -1;

  /* copy from user space into file buffer */
  memcpy (fops_file_pointer[FD2NO] + FD2INDEX, __buf, copy_size);
  FD2INDEX = FD2INDEX + copy_size;

  /* set file lenght for next read */
  fops_file_lenght[__fd] = FD2INDEX;
  return copy_size;
}


/*
 * seek
 */
long
io_grubfs_lseek (int __fd, long __offset, int __whence)
{
  /* invalid index */
  if (__fd >= MAXFILES_OPEN)
    return -1;

  /* adjust index */
  switch (__whence)
  {
  case SEEK_SET:
    FD2INDEX = __offset;
    break;
  case SEEK_CUR:
    FD2INDEX = FD2INDEX + __offset;
    break;
  case SEEK_END:
    FD2INDEX = fops_file_lenght[FD2NO] + __offset;
    break;
  }
  return FD2INDEX;
}

/*
 * fstat - size only
 */
int
io_grubfs_fstat (int __fd, struct stat *buf)
{
  /* invalid index */
  if (__fd >= MAXFILES_OPEN)
    return -1;


  buf->st_size = fops_file_lenght[FD2NO];
  return 0;
}


/*
 * not implemented yet
 */
int
io_grubfs_ftruncate (int __fd, unsigned long __offset)
{
  printf ("grubfs: ftruncate() not implemented !!\n");
  return -1;
}

/*
 *    internal functions for access to grubfs
 */
static int
io_grubfs_init_grub_mods (void)
{
  int err;

  printf ("grubfs: %d module(s) (only %i or less usable):\n",
	  l4env_multiboot_info->mods_count, MAXFILES);

  for (err = 0; err < l4env_multiboot_info->mods_count; err++)
  {
    l4util_mb_mod_t *mod =
      (l4util_mb_mod_t *) (l4env_multiboot_info->mods_addr +
			   (err * sizeof (l4util_mb_mod_t)));

    printf ("grubfs:   [%d] @ %p\n", err, (void *) mod->mod_start);

    fops_file[err][0] = mod->mod_start;
    fops_file[err][1] = mod->mod_end;
    fops_file_no++;
    printf ("grubfs: init file %i with %d bytes from %p to %p\n", err,
	    fops_file[err][1] - fops_file[err][0], (void *) fops_file[err][0],
	    (void *) fops_file[err][1]);

    if (err == MAXFILES - 1)
      break;
  }

  return 0;
}

/*
 * translation from filename to filedes 
 */
static int
name2fileno (const char *__name)
{
  int file_no;

  /* check old style names "one" to "four" */
  if ((!strncasecmp ("one", __name, 3)) && (fops_file_no >= 1))
    return 0;
  if ((!strncasecmp ("two", __name, 3)) && (fops_file_no >= 2))
    return 1;
  if ((!strncasecmp ("three", __name, 5)) && (fops_file_no >= 3))
    return 2;
  if ((!strncasecmp ("four", __name, 4)) && (fops_file_no >= 4))
    return 3;

  /* try to get ame out of int */
  file_no = atoi (__name);
  file_no--;

  /* name hast do be an int < MAXFILES */
  if ((file_no >= MAXFILES) || (file_no > fops_file_no))
    return -1;

  /* valid id */
  return file_no;
}


/*
 * init grub modules
 */
static int
io_grubfs_init ()
{
  int i, j;
  unsigned char *c;

  /* mark all files as closed */
  for (i = 0; i < MAXFILES_OPEN; i++)
    file_list[i].file_no = -1;

  /* Initilize grub_mods */
  if (fops_file_no == 0)
    io_grubfs_init_grub_mods ();
  if (fops_file_no == 0)
  {
    printf ("grubfs: NO FILES FOUND !!");
    return -1;
  }

  /*
   * the files loaded via grub are x-kB aligned!
   * file is appended by 0x0 to next full x-kB, isn't it?
   * this doesn't always work!
   */
  for (i = 0; i < fops_file_no; i++)
  {
    c = fops_file_pointer[i] = (unsigned char *) fops_file[i][0];
    fops_file_size[i] = fops_file[i][1] - fops_file[i][0];
    /* try to get real filesize by searching the "last != 0x0" */
    j = fops_file_size[i] - 1;
    while ((c[j] == '\0') && (j >= 0))
      j--;
    fops_file_lenght[i] = j + 1;
  }

  return 0;
}
