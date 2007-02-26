
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

exc_img_t::exc_img_t(const char *fname, l4env_infopage_t *env)
  : vaddr(0)
{
  set_pathname(fname, env);
}

/** Set pathname of image. If necessary, prepend it by a path */
void
exc_img_t::set_pathname(const char *fname, l4env_infopage_t *env)
{
  unsigned int fname_len;
  unsigned int pathname_len;
  
  if (strlen(fname) > EXC_MAXFNAME)
    Panic("file name too long: %s", fname);
  
  *pathname = '\0';

  if (*fname != '(' && *fname != '/')
    {
      strncpy(pathname, 
	      (!strncmp(fname, "lib", 3)) ? env->libpath : env->binpath,
	      sizeof(pathname)-1);
      pathname[sizeof(pathname)-1] = '\0';

      if (   ((pathname_len = strlen(pathname)) > 0)
	  &&  (pathname_len < sizeof(pathname)-1)
	  &&  (pathname[pathname_len-1] != '/'))
	{
	  pathname[pathname_len  ] = '/';
	  pathname[pathname_len+1] = '\0';
	}
    }

  // append filename
  pathname_len = strlen(pathname);
  fname_len = strlen(fname);
  strncat(pathname, fname, min(sizeof(pathname)-1-pathname_len, fname_len));
  
  // store fname identifier
  set_fname(pathname);
}

/** Request ELF object from file provider
 *
 * \param fname		name of ELF file to load
 * \param img		ELF image descriptor
 * \param env		L4 environment infopage
 * \return		0 on success */
int
exc_img_t::load(l4env_infopage_t *env)
{
  int error;
  sm_exc_t exc;
  l4dm_dataspace_t ds;

  msg("Loading");

  /* ask the file provider server for the plain file image */
  if (   (error = check(l4fprov_file_open(env->fprov_id, pathname,
				         (l4fprov_threadid_t*)&env->image_dm_id,
					 0, (l4fprov_dataspace_t*)&ds, 
					 &size, &exc),
			"from file provider"))
      /* attach the dataspace to the memory region */
      || (error = check(l4rm_attach(&ds, size, 0, 
			L4DM_RO | L4RM_LOG2_ALIGNED, (void**)&vaddr),
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

