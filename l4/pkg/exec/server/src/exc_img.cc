/* $Id$ */
/**
 * \file	exec/server/src/exc_img.cc
 * \brief	class for dealing with binary images
 *
 * \date	10/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "exc_img.h"

#include "assert.h"

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>

#include <string.h>

#include <l4/dm_mem/dm_mem.h>

extern "C" {
#include <l4/generic_fprov/generic_fprov-client.h>
}

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

exc_img_t::exc_img_t(const char *fname, const l4dm_dataspace_t *ds, 
		     l4env_infopage_t *env)
  : vaddr(0)
{
  set_names(fname, env);
  img_ds = *ds;

  if (!l4dm_is_invalid_ds(img_ds))
    {
      int error;

      if (((error = check(l4dm_mem_size(&img_ds, &size),
	           "determining size"))) ||
          ((error = check(l4rm_attach(&img_ds, size, 0, L4DM_RO, 
				      (void**)&vaddr),
		   "attaching region"))))
	{
	  img_ds = L4DM_INVALID_DATASPACE;
	}
    }
}

/**\brief Set path of image.
 *
 * If we have to use the library path or the binary depends on the name
 * of the image. If it begins with "lib", we use the library path, else
 * we use the binary path.
 */
void
exc_img_t::set_names(const char *fname, l4env_infopage_t *env)
{
  Assert(fname);
  Assert(env);

  int len = strlen(fname);
  if(len > (int)sizeof(pathname)-1){
      printf("file name too long: %s\n", fname);
      set_fname("");
  } else {
      memcpy(pathname, fname, len+1);
      // store fname identifier
      set_fname(pathname);
  }
}


/** Request ELF object from file provider
 *
 * \param env		L4 environment infopage
 * \return		0 on success */
int
exc_img_t::load(l4env_infopage_t *env)
{
  int error = -L4_ENOTFOUND;
  CORBA_Environment _env = dice_default_environment;
  l4dm_dataspace_t ds = img_ds;

  msg("Loading");

  /* ask the file provider server for the plain file image */
  if (l4dm_is_invalid_ds(img_ds))
    {
	char filename[EXC_MAXFNAME];
	const char *path;

	if(pathname[0]!='(' && pathname[0]!='/') {
	    path = (!strncmp(pathname, "lib", 3)) ? env->libpath 
	                                          : env->binpath;
	} else {
	    path = "";
	}
	do{
	    unsigned l, ln;
	    char *colon;

	    colon = strchr(path, ':');
	    l = colon ? colon-path : strlen(path);
	    if(l>EXC_MAXFNAME-2){
		printf("Skipping long path %*s\n", l, path);
		path = colon ? colon+1 : path + l;
		continue;
	    }
	    memcpy(filename,path,l);
	    path = colon? colon+1 : path+l;
	    if(l)filename[l++]='/';
	    ln = strlen(pathname)+1;
	    if(l+ln>EXC_MAXFNAME){
		printf("Skipping long path+file %s/...\n", filename);
		continue;
	    }
	    memcpy(filename+l, pathname, ln);
	    
	    error = l4fprov_file_open_call(&(env->fprov_id), filename,
					   &env->image_dm_id,
					   0, &ds, 
					   &size, &_env);
	    if(!error){
		memcpy(pathname, filename, l+ln);
		break;
	    }
	} while(*path);
	
	if( check(error, "from file provider, path was \"%s\".",
		  (!strncmp(pathname, "lib", 3)) ? env->libpath 
		                                 : env->binpath))
	    return error;
    }

  /* attach the dataspace to the memory region */
  if ((error = check(l4rm_attach(&ds, size, 0, 
				 L4DM_RO | L4RM_LOG2_ALIGNED, 
			 	 (void**)&vaddr),
	  "attaching ds")))
    return error;

#ifdef DEBUG_LOAD
  msg("Loaded image to %08x-%08x", vaddr, vaddr+size);
#endif

  return 0;
}

// destructor: close and detach dataspace
exc_img_t::~exc_img_t()
{
  if (vaddr != 0)
    {
      l4dm_mem_release(vaddr);
    }
}

