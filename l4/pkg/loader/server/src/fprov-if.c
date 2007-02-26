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

#include <stdio.h>
#include <string.h>

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
 * \param fname		file name
 * \param use_modpath	if !=0, expand fname with cfg_modpath
 * \param contiguos	if !=0, ask file provider to create contiguos ds
 * \param fprov_id	id of file provider
 * \param dm_id		id of dataspace manager
 * \retval addr		Address the image was loaded to. If NULL, don't attach
 *                      the image to our address space.
 * \retval size		size (in bytes) of the file as returned by fprov
 * \retval ds		associated dataspace
 * \retval rg		associated region of L4 region manager
 * \return		0 on success */
int
load_file(const char *fname, l4_threadid_t fprov_id, l4_threadid_t dm_id,
	  int use_modpath, int contiguous, 
	  l4_addr_t *addr, l4_size_t *size, l4dm_dataspace_t *ds)
{
  int error = -L4_ENOTFOUND;
  CORBA_Environment _env = dice_default_environment;
  const char *path;

  path = (use_modpath && *fname!='(' && *fname!='/') ? cfg_modpath : "";
  do 
    {
      static char pathname[255];
      unsigned l, ln;
      const char *colon;
      
      colon = strchr(path, ':');
      l = colon ? colon-path : strlen(path);
      if (l > sizeof(pathname)-2)
	{
	  printf("Skipping long path %*s\n", l, path);
	  path = colon ? colon+1 : path+l;
	  continue;
	}
      memcpy(pathname, path, l);
      path = colon ? colon+1 : path+l;
      if (l)
	pathname[l++]='/';
      ln = strlen(fname)+1;
      if(l+ln>sizeof(pathname))
	{
	  printf("Skipping long path+file %s/%s\n",
		 pathname, fname);
	  continue;
	}
      memcpy(pathname+l, fname, ln);

      /* get file image by file provider */
      if (!(error = l4fprov_file_open_call(&fprov_id, pathname, &dm_id,
					   contiguous ? L4DM_CONTIGUOUS : 0,
					   ds, size, &_env)))
	break;

    } while(*path);

  if (error)
    {
      printf("Error %d opening file \"%s\"\n", error, fname);
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

/** init file provider stuff */
int
fprov_if_init(void)
{
  l4_threadid_t id;

  /* file provider */
  if (!names_waitfor_name("TFTP", &id, 10000))
    {
      printf("TFTP not found\n");
      return -L4_ENOTFOUND;
    }
  tftp_id = id;
  
  return 0;
}

