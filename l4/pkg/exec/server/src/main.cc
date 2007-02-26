/* $Id$ */
/**
 * \file	server/src/main.cc
 * \brief	Initialization and main server loop
 *
 * \date	08/18/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */

#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/exec/exec.h>
#include <l4/exec/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/oskit10_l4env/support.h>

#include <stdio.h>

extern "C" {
#include "exec-server.h"
}

#include "exc.h"
#include "bin_obj.h"
#include "config.h"
#include "assert.h"
#include "check.h"


/** Check if there was an Linker error. We only do that if there was
 * not another error. */
static int
check_link_errors(int error, l4env_infopage_t *env)
{
  /* in case there are no errors search for possible link errors */
  if (!error)
    {
      int i;
      for (i=0; i<env->section_num; i++)
	{
	  if (env->section[i].info.type & L4_DSTYPE_ERRLINK)
	    {
	      error = -L4_EXEC_LINK;
	      break;
	    }
	}
    }
  
  return error;
}

/** Open new EXC module and split sections into dataspaces.
 * 
 * \param request  pointer to Flick request structure
 * \param pathname filename (with path) of module to load
 * \param envpage  environment page
 * \param _ev      Flick exception structure (unused)
 *
 * \attention environment page is updated
 * \return 0 on success, -L4_ENOMEM if allocation failed */
l4_int32_t
l4exec_bin_server_open(sm_request_t *request, const char *pathname,
		       l4exec_envpage_t envpage, l4_uint32_t flags,
		       sm_exc_t *_ev)
{
  int error;
  unsigned int id;
  l4env_infopage_t *env = (l4env_infopage_t*)envpage;
  bin_obj_t *bin_obj;
  dsc_obj_t **bin_obj_dsc;
  exc_obj_t *exc_obj;

  /* allocate anchor */
  if ((error = check(bin_objs->alloc(&bin_obj_dsc, &id),
		     "allocating binary descriptor")))
    /* no more descriptor available: either kill one binary object
     * or increase the static allocated array of binary descriptors
     * (EXC_MAXBIN) */
    return error;
  
  /* allocate object */
  if (!(bin_obj = new bin_obj_t(id)))
    {
      printf("Can't create binary object\n");
      bin_objs->free(id);
      return -L4_ENOMEM;
    }

  /* set anchor */
  *bin_obj_dsc = bin_obj;
 
  /* init L4 environment infopage */
  ::exc_init_env(bin_obj->get_id(), env);

  /* translate exc_obj create flags */
  int new_flags =  ((flags & L4EXEC_LOAD_SYMBOLS) ? EO_LOAD_SYMBOLS : 0)
		  |((flags & L4EXEC_LOAD_LINES)   ? EO_LOAD_LINES   : 0)
		  |((flags & L4EXEC_DIRECT_MAP)   ? EO_DIRECT_MAP   : 0);

  if (/* Scan image, create exc_obj according to file format.
       * Image will be loaded regardless if we loaded it earlier. */
      (error = ::exc_obj_load_bin(pathname, 1, request->client_tid, new_flags,
				  &exc_obj, env)) ||

      /* tell bin_obj it's name */
      (error = bin_obj->set_bin(exc_obj)) ||

      /* load libraries the binary depends on */
      (error = bin_obj->load_libs(env)))

    /* any error occured, delete binary */
    {
      delete bin_obj;
      return error;
    }

  /* At this point, the binary is loaded successfully and bin_obj is valid. */

  if (/* Try to do a first link step. At this time we only link the library
       * libloader.s.so. */
      (error = bin_obj->link_first(env)) ||

      /* Mark libloader as startup library */
      (error = bin_obj->mark_startup_library(env)) ||

      /* Search the first program entry.
       * We search for "libloader.s::l4loader_init" */
      (error = bin_obj->set_1st_entry(env)))
    {
      /* 1st link failed or special entry point not found. If the error 
       * was that libloader.s was not found, then try to use Plan B. It still 
       * may be a dynamic linkable app */
      if (error == -L4_EXEC_NOSTANDARD)
	{
      	  env->entry_1st = L4_MAX_ADDRESS;
	  env->entry_2nd = bin_obj->get_entry();
	  error = 0;
	}
    }

  /* in case there are no errors search for possible link errors */
  error = check_link_errors(error, env);

  return error;
}

/** Close a binary. Free all occupied ressources.
 * 
 * \param request pointer to Flick request structure
 * \param envpage environment page
 * \param _ev     Flick exception structure (unused)
 * 
 * \attention environment page is updated
 * \return 0 on success, -L4_ENOTFOUND if invalid envpage */
l4_int32_t
l4exec_bin_server_close(sm_request_t *request,
			l4exec_envpage_t envpage, sm_exc_t *_ev)
{
  l4env_infopage_t *env = (l4env_infopage_t*)envpage;
  bin_obj_t *bin_obj = static_cast<bin_obj_t*>(bin_objs->lookup(env->id));

  /* binary object not found */
  if (!bin_obj)
    {
      Error("binary object with id=%d not found\n", env->id);
      return -L4_ENOTFOUND;
    }

  /* delete binary */
  delete bin_obj;
  
  /* free/deatch dataspaces of envpage */
  exc_obj_psec_kill_from_env(env);

  return 0;
}

/** Relocate a previous opened binary.
 * 
 * \param request pointer to Flick request structure
 * \param envpage environment page
 * \param _ev     Flick exception structure (unused)
 * 
 * \attention environment page is updated
 * \return 0 on success, -L4_ENOMEM if allocation failed */
l4_int32_t
l4exec_bin_server_link(sm_request_t *request,
		       l4exec_envpage_t envpage, sm_exc_t *_ev)
{
  int error;
  l4env_infopage_t *env = (l4env_infopage_t*)envpage;
  bin_obj_t *bin_obj = static_cast<bin_obj_t*>(bin_objs->lookup(env->id));

  if (!bin_obj)
    {
      Error("binary object with id=%d not found\n", env->id);
      return -L4_ENOTFOUND;
    }
  
  if ((error = bin_obj->link(env)) ||
      (error = bin_obj->set_2nd_entry(env)))
    ;
  
  if (error == -L4_EXEC_NOSTANDARD)
    {
      /* set entry_2nd again -- it might be changed during relocation */
      env->entry_2nd = bin_obj->get_entry();
      error = 0;
    }
  
  /* in case there are no errors search for possible link errors */
  error = check_link_errors(error, env);
  
  return error;
}

/** Get debug symbols of binary object
 *
 * \param request pointer to Flick request structure
 * \param envpage environment page
 * \retval sym_ds dataspace containing symbols
 * \param _ev     Flick exception structure (unused)
 * \return 	0 on success 
 * 		-L4_NOTFOUND no symbols available or invalid L4 envpage */
l4_int32_t
l4exec_bin_server_get_symbols(sm_request_t *request,
			      l4exec_envpage_t envpage, 
			      l4exec_dataspace_t *sym_ds, sm_exc_t *_ev)
{
  int error;
  l4env_infopage_t *env = (l4env_infopage_t*)envpage;
  bin_obj_t *bin_obj = static_cast<bin_obj_t*>(bin_objs->lookup(env->id));

  if (!bin_obj)
    {
      Error("binary object with id=%d not found\n", env->id);
      return -L4_ENOTFOUND;
    }
  
  if (!(error = bin_obj->get_symbols(env, (l4dm_dataspace_t*)sym_ds)))
    /* make dataspace accessible to caller */
    l4dm_transfer((l4dm_dataspace_t*)sym_ds, request->client_tid);
  
  return error;
}

/** Get debug lines of binary object
 *
 * \param request pointer to Flick request structure
 * \param envpage environment page
 * \retval sym_ds dataspace containing symbols
 * \param _ev     Flick exception structure (unused)
 * \return 	0 on success 
 * 		-L4_NOTFOUND no symbols available or invalid L4 envpage */
l4_int32_t
l4exec_bin_server_get_lines(sm_request_t *request,
			    l4exec_envpage_t envpage, 
			    l4exec_dataspace_t *lines_ds, sm_exc_t *_ev)
{
  int error;
  l4env_infopage_t *env = (l4env_infopage_t*)envpage;
  bin_obj_t *bin_obj = static_cast<bin_obj_t*>(bin_objs->lookup(env->id));

  if (!bin_obj)
    {
      Error("binary object with id=%d not found\n", env->id);
      return -L4_ENOTFOUND;
    }
  
  if (!(error = bin_obj->get_lines(env, (l4dm_dataspace_t*)lines_ds)))
    /* make dataspace accessible to caller */
    l4dm_transfer((l4dm_dataspace_t*)lines_ds, request->client_tid);
  
  return error;
}

/** Server Loop function */
static void
server_loop(void)
{
  int ret;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;

  /* enter request loop */
  flick_init_request(&request, &ipc_buf);
  for (;;)
    {
      result = flick_server_wait(&request);
      while (!L4_IPC_IS_ERROR(result))
	{
	  ret = l4exec_bin_server(&request);
#if DEBUG_REQUEST
	  LOGI("request 0x%08x, src %x.%x\n", ipc_buf.buffer[0],
	      request.client_tid.id.task, request.client_tid.id.lthread);
#endif
	  switch (ret)
	    {
	    case DISPATCH_ACK_SEND:
	      result = flick_server_reply_and_wait(&request);
	      break;

	    default:
	      LOGI("Flick dispatch error (%d)!\n", ret);
	      result = flick_server_wait(&request);
	      break;
	    }
	}
      printf("Flick IPC error (0x%08x, src=%x.%x)!\n",
	      result.msgdope, request.client_tid.id.task,
	                      request.client_tid.id.lthread);
      enter_kdebug("flick ipc error");
    }
}

/** Main function
 * 
 * \param argc		number of program arguments
 * \param argv		program arguments array */
int
main (int argc, char **argv)
{
  LOG_init("exec");

  /* dymanic allocation with max = HEAP_SIZE */
  OSKit_libc_support_init(-HEAP_SIZE);

  if (   !(exc_objs = new dsc_array_t(EXC_MAXOBJ))
      ||  (exc_objs->init())
      || !(bin_objs = new dsc_array_t(EXC_MAXBIN))
      ||  (bin_objs->init()))
    {
      printf("out of memory\n");
      return -1;
    }

  /* we provide a service */
  if (!names_register("EXEC"))
    {
      printf("failed to register EXEC\n");
      return -1;
    }

  server_loop();

  return 0;
}

