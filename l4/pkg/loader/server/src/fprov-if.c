/* $Id$ */
/**
 * \file	loader/server/src/fprov-if.c
 *
 * \date	06/11/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Helper functions for communicating with file provider */

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
  int error;
  sm_exc_t exc;

  /* if neccessary, extent filename by modpath */
  if (use_modpath && *cfg_modpath && *fname != '(' && *fname != '/')
    {
      static char pathname[255];
      int pathname_len, fname_len;
      
      strncpy(pathname, cfg_modpath, sizeof(pathname)-1);
      pathname[sizeof(pathname)-1]='\0';
      
      if (((pathname_len=strlen(pathname))>0)
	  && (pathname_len<sizeof(pathname)-1)
	  && (pathname[pathname_len-1]!='/'))
	{
	  /* append '/' */
	  pathname[pathname_len]='/';
	  pathname[pathname_len+1]='\0';
	}
      
      pathname_len = strlen(pathname);
      fname_len = strlen(fname);
      strncat(pathname, fname, min(sizeof(pathname)-1-pathname_len, fname_len));

      fname = pathname;
    }

  /* get file image by file provider */
  if ((error = l4fprov_file_open(fprov_id, fname,
				 (l4fprov_threadid_t*)&dm_id, 
				 contiguous ? L4DM_CONTIGUOUS : 0,
				 (l4fprov_dataspace_t*)ds, size, &exc)))
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

