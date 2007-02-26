/* $Id$ */
/**
 * \file	loader/server/src/fprov-if.c
 * \brief	Helper functions for communicating with file provider
 *
 * \date	06/11/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "fprov-if.h"
#include "cfg.h"
#include "global.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <l4/env/errno.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-client.h>

l4_threadid_t tftp_id = L4_INVALID_ID;	     /* tftp server */

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef max
#define max(x,y) ((x)>(y)?(x):(y))
#endif

/** Get file from file provider.
 *
 * \param fname_and_arg file name (optional arguments are ignored)
 * \param search_path	colon delemited paths to try to load the file
 * \param contiguous	if !=0, ask file provider to create contiguos ds
 * \param fprov_id	id of file provider
 * \param dsm_id	id of dataspace manager
 * \retval addr		Address the image was loaded to. If NULL, don't attach
 *                      the image to our address space.
 * \retval size		size (in bytes) of the file as returned by fprov
 * \retval ds		associated dataspace
 * \return		0 on success */
int
load_file(const char *fname_and_arg,
	  l4_threadid_t fprov_id, l4_threadid_t dsm_id,
	  const char *search_path, int contiguous,
	  l4_addr_t *addr, l4_size_t *size, l4dm_dataspace_t *ds)
{
  DICE_DECLARE_ENV(_env);
  char *pathname = malloc(MAX_PATHLEN);
  l4_size_t fname_len = strlen(fname_and_arg);
  const char *path, *o;
  int error = -L4_ENOTFOUND;

  if (!pathname)
    {
      printf("Malloc error\n");
      return error;
    }

  if ((o = strchr(fname_and_arg, ' ')))
    fname_len = o-fname_and_arg;

  if (!fname_len || !fname_and_arg)
    {
      printf("Error. No file name given.\n");
      return error;
    }

  path = (search_path && *fname_and_arg!='(' && *fname_and_arg!='/')
	    ? search_path
	    : "";

  do
    {
      l4_size_t l, ln;
      const char *colon;

      colon = strchr(path, ':');
      l = colon ? colon-path : strlen(path);
      if (l > MAX_PATHLEN-2)
	{
	  printf("Skipping long path %*.*s\n", l, l, path);
	  path = colon ? colon+1 : path+l;
	  continue;
	}
      memcpy(pathname, path, l);
      path = colon ? colon+1 : path+l;
      if (l && pathname[l-1] != '/')
	pathname[l++]='/';
      ln = fname_len+1;
      if(l+ln>MAX_PATHLEN)
	{
	  printf("Skipping long path+file %s/%*.*s\n",
		 pathname, fname_len, fname_len, fname_and_arg);
	  continue;
	}
      snprintf(pathname+l, ln, "%s", fname_and_arg);

      /* get file image by file provider */
      if (!(error = l4fprov_file_open_call(&fprov_id, pathname, &dsm_id,
					   contiguous ? L4DM_CONTIGUOUS : 0,
					   ds, size, &_env)))
	break;

    } while (*path);

  free(pathname);

  if (error)
    {
      printf("Error %d opening file \"%*.*s\"\n",
	  error, fname_len, fname_len, fname_and_arg);
      return error;
    }

  if (addr)
    {
      /* attach dataspace to region manager */
      if ((error = l4rm_attach(ds, *size, 0, L4DM_RW, (void **)addr)))
	{
	  printf("Error %d attaching ds size %d\n", error, *size);
	  return error;
	}
    }

  return 0;
}
