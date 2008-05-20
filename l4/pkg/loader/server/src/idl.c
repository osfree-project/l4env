/* $Id$ */
/**
 * \file	loader/server/src/idl.c
 * \brief	implemented IDL interface support functions
 *
 * \date	06/11/2001
 * \author	Frank Mehnert */

/* (c) 2003-2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "idl.h"

#include <stdio.h>
#include <l4/env/errno.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/util.h>

#include "cfg.h"
#include "app.h"
#include "dm-if.h"
#include "lib.h"

static char error_message[MAX_ERROR_MSG];


/** write error message in buffer to return to client */
int
return_error_msg(int error, const char * const msg, const char *fname)
{
  if (error < 0)
    snprintf(error_message, sizeof(error_message),
	     "Error %d (%s) %s of %s", error, l4env_errstr(error), msg, fname);
  return error;
}

/** Load application
 *
 * \param client	caller
 * \param img_ds	dataspace containing the script or binary
 * \param fname		file name
 * \param fprov		file provider to get the file image from
 * \param flags		flags (currently unused)
 * \retval task_ids	IDs of started tasks
 * \retval error_msg	error message
 * \retval _env		IDL exception structure
 * \return		0 on success */
long
l4loader_app_open_component (CORBA_Object _dice_corba_obj,
                             const l4dm_dataspace_t *img_ds,
                             const char* fname,
                             const l4_threadid_t *fprov,
                             unsigned long flags,
                             l4_taskid_t task_ids[16],
                             char* *error_msg,
                             CORBA_Server_Environment *_dice_corba_env)
{
  int ret = 0;
  int is_binary = 0;
  void *addr = 0;
  l4_size_t size;
  l4dm_dataspace_t *ds = (l4dm_dataspace_t*)img_ds;

  error_msg[0] = '\0';

  if (l4dm_is_invalid_ds(*ds))
    ret = load_config_script_from_file(fname, *fprov, *_dice_corba_obj,
				       flags, task_ids);

  else
    {
      if ((ret = l4dm_mem_size(ds, &size)))
	{
	  return_error_msg(ret, "determining size", fname);
	  goto error;
	}

      if ((ret = l4rm_attach(ds, size, 0, L4DM_RO, &addr)))
	{
	  return_error_msg(ret, "attaching ds", fname);
	  goto error;
	}

      ret = load_config_script(fname, *fprov, ds, (l4_addr_t)addr,
			       size, *_dice_corba_obj, flags, &is_binary, task_ids);

error:
      if (!is_binary)
	junk_ds((l4dm_dataspace_t*)img_ds, (l4_addr_t)addr);
    }

  *error_msg = error_message;
  return ret;
}


/** Continue application which was stopped just before the real start.
 * \param client	caller
 * \param taskid	ID of task to continue as returned by open()
 * \retval _env		IDL exception structure */
long
l4loader_app_cont_component (CORBA_Object _dice_corba_obj,
                             const l4_taskid_t *taskid,
                             CORBA_Server_Environment *_dice_corba_env)
{
  app_t *app = task_to_app(*taskid);
  if (!app)
    return -L4_ENOTFOUND;

  return app_cont(app);
}


/** Kill application
 *
 * \param client	caller
 * \param task_id	ID of task to kill as returned by open()
 * \param flags		flags (currently unused)
 * \retval _env		IDL exception structure
 * \return		0 on success */
long
l4loader_app_kill_component (CORBA_Object _dice_corba_obj,
                             const l4_taskid_t *task_id,
                             unsigned long flags,
                             CORBA_Server_Environment *_dice_corba_env)
{
  if (task_id == NULL)
    return -L4_EINVAL;

  return app_kill(*task_id, *_dice_corba_obj);
}

/** Dump application
 *
 * \param client	caller
 * \param task_id	id of task to dump
 * \param flags		flags (currently unused)
 * \retval _env		IDL exception structure
 * \return		0 on success */
long
l4loader_app_dump_component (CORBA_Object _dice_corba_obj,
                             unsigned long task_id,
                             unsigned long flags,
                             CORBA_Server_Environment *_dice_corba_env)
{
  return app_dump(task_id);
}

/** Get application info
 *
 * \param client	caller
 * \param task_id	id of task to get info from
 * \param flags		flags (currently unused)
 * \retval fname	application name
 * \retval l4env_page	L4 environment infopage of process
 * \retval _env		IDL exception structure
 * \return		0 on success */
long
l4loader_app_info_component (CORBA_Object _dice_corba_obj,
                             unsigned long task_id,
                             unsigned long flags,
                             char* *fname,
                             l4dm_dataspace_t *l4env_page,
                             CORBA_Server_Environment *_dice_corba_env)
{
  return app_info(task_id, l4env_page, *_dice_corba_obj, fname);
}

/** IDL server loop */
void
server_loop(void)
{
  l4loader_app_server_loop(NULL);
  l4_sleep_forever();
}
