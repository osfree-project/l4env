/* $Id$ */
/**
 * \file	loader/server/src/exec-if.c
 * \brief	Helper functions for communication with exec layer
 *
 * \date	05/21/2003
 * \author	Frank Mehnert */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>

#include <l4/names/libnames.h>
#include <l4/exec/exec.h>
#include <l4/exec/exec-client.h>
#include <l4/env/errno.h>
#include <l4/dm_phys/dm_phys.h>

#include "app.h"
#include "exec-if.h"

static l4_threadid_t exec_id = L4_INVALID_ID;

/** @name Manipulate Fiasco debug information */
/*@{ */
/** Request symbolic information for an application from the L4 exec layer */
int
exec_if_get_symbols(app_t *app)
{
  int error;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_size_t psize;
  CORBA_Environment _env = dice_default_environment;
  
  if ((error = l4exec_bin_get_symbols_call(&exec_id, 
	                                   (long*)app->env,
				            &ds, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      app_msg(app, "Error %d (%s) getting symbols", 
	      error, l4env_errstr(error));
      return -L4_EINVAL;
    }

  if (l4dm_is_invalid_ds(ds))
    {
      app_msg(app, "No symbols");
      return -L4_EINVAL;
    }

  if (!l4dm_mem_ds_is_contiguous(&ds))
    {
      app_msg(app, "ds %d not contiguous (bug at exec)!", ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }
  
  if ((error = l4dm_mem_ds_phys_addr(&ds, 0, L4DM_WHOLE_DS, &addr, &psize)))
    {
      app_msg(app, "Error %d (%s) requesting physical addr of ds %d", 
	      error, l4env_errstr(error), ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }

  app->symbols_ds = ds;   /* needed for transfering ownership to app when
			   * we know the task_id */
  app->symbols    = addr; /* physical address */

  return 0;
}

/** Request line number information of an application by the L4 exec layer */
int
exec_if_get_lines(app_t *app)
{
  int error;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_size_t psize;
  CORBA_Environment _env = dice_default_environment;
  
  if ((error = l4exec_bin_get_lines_call(&exec_id,
	                            (long*)app->env,
				    &ds, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      app_msg(app, "Error %d (%s) getting lines", 
	            error, l4env_errstr(error));
      return -L4_EINVAL;
    }

  if (l4dm_is_invalid_ds(ds))
    {
      app_msg(app, "No lines");
      return -L4_EINVAL;
    }

  if (!l4dm_mem_ds_is_contiguous(&ds))
    {
      app_msg(app, "ds %d not contiguous (bug at exec)!", ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }

  if ((error = l4dm_mem_ds_phys_addr(&ds, 0, L4DM_WHOLE_DS, &addr, &psize)))
    {
      app_msg(app, "Error %d (%s) requesting physical addr of ds %d", 
	            error, l4env_errstr(error), ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }

  app->lines_ds = ds;   /* needed for transfering ownership to app when we
			 * know the task_id */
  app->lines    = addr; /* physical address */

  return 0;
}

int
exec_if_link(app_t *app)
{
  int error;
  CORBA_Environment _env = dice_default_environment;

  if ((error = l4exec_bin_link_call(&exec_id, 
				    (l4exec_envpage_t*)app->env, 
				    &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      app_msg(app, "Error %d (%s) while linking (exc=%d)",
		   error, l4env_errstr(error), _env.major);
      return error;
    }

  return 0;
}

int
exec_if_open(app_t *app, const char *fname, l4dm_dataspace_t *ds, 
	     int open_flags)
{
  int error;
  CORBA_Environment _env = dice_default_environment;

  if (!l4dm_is_invalid_ds(*ds))
    {
      /* Valid dataspace passed. Use it as binary image. Pass ownership
       * of dataspace to exec server. It will be also destroyed there. */
      if ((error = l4dm_transfer(ds, exec_id)) < 0)
	{
	  app_msg(app, "Error %d (%s) transfering ds to exec",
		      error, l4env_errstr(error));
	  l4dm_close(ds);
	  return error;
	}
    }

  if ((error = l4exec_bin_open_call(&exec_id, fname, ds,
				    (l4exec_envpage_t*)app->env, 
				    open_flags, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      app_msg(app, "Error %d (%s) while loading",
		  error, l4env_errstr(error));
      return error;
    }

  return 0;
}

int
exec_if_close(app_t *app)
{
  int error;
  CORBA_Environment _env = dice_default_environment;

  if ((error = l4exec_bin_close_call(&exec_id,
				     (l4exec_envpage_t*)app->env, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      app_msg(app, "Error %d (%s) deleting task at exec server",
		   error, l4env_errstr(error));
      return error;
    }

  return 0;
}

int
exec_if_ftype(l4dm_dataspace_t *ds, l4env_infopage_t *env)
{
  int error;
  CORBA_Environment _env = dice_default_environment;

  if ((error = l4dm_share(ds, exec_id, L4DM_RO)))
    {
      printf("Error %d (%s) sharing rights to exec\n",
	     error, l4env_errstr(error));
      return -L4_EINVAL;
    }

  if (((error = l4exec_bin_ftype_call(&exec_id, ds, 
				     (l4exec_envpage_t*)env, &_env)) < 0)
      || _env.major != CORBA_NO_EXCEPTION)
    {
      printf("Error %d (%s) checking filetype at exec server\n",
	     error, l4env_errstr(error));
      return -L4_EINVAL;
    }

  return error;
}

int
exec_if_init(void)
{
  l4_threadid_t id;

  /* ELF loader */
  if (!names_waitfor_name("EXEC", &id, 5000))
    {
      printf("EXEC not found\n");
      return -1;
    }
  exec_id = id;

  return 0;
}

