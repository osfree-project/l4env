/* $Id$ */
/**
 * \file	loader/server/src/idl.c
 * \brief	implemented IDL interface support functions
 *
 * \date	06/11/2001
 * \author	Frank Mehnert */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "idl.h"

#include <stdio.h>
#include <l4/env/errno.h>

#include "cfg.h"
#include "app.h"

static char error_message[MAX_ERROR_MSG];

/** Load application
 *
 * \param request	idl request structure
 * \param fname		file name of application
 * \param fprov		id of file provider
 * \param flags		flags (currently unused)
 * \param error_msg	error message
 * \param _ev		flick exception structure
 * \return		0 on success */
l4_int32_t 
l4loader_app_open_component(CORBA_Object _dice_corba_obj,
    const char* fname,
    const l4_threadid_t *fprov,
    l4_uint32_t flags,
    char ** error_msg,
    CORBA_Environment *_dice_corba_env)
{
  int ret;

  error_msg[0] = '\0';
  ret = load_config_script(fname, *fprov);
  *error_msg = error_message;
  return ret;
}

/** Kill application
 *
 * \param request	idl request structure
 * \param fname		file name of application to kill
 * \param flags		flags (currently unused)
 * \return		0 on success */
l4_int32_t 
l4loader_app_kill_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t task_id,
    l4_uint32_t flags,
    CORBA_Environment *_dice_corba_env)
{
  return app_kill(task_id);
}

/** Dump application
 *
 * \param request	idl request structure
 * \param task_id	id of task to dump
 * \param flags		flags (currently unused)
 * \return		0 on success */
l4_int32_t 
l4loader_app_dump_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t task_id,
    l4_uint32_t flags,
    CORBA_Environment *_dice_corba_env)
{
  return app_dump(task_id);
}

/** Get application info
 * 
 * \param request	idl request structure
 * \param task_id	id of task to get info from
 * \param flags		flags (currently unused)
 * \retval fname	application name
 * \retval l4env_page	L4 environment infopage of process
 * \return		0 on success */
l4_int32_t 
l4loader_app_info_component(CORBA_Object _dice_corba_obj,
    l4_uint32_t task_id,
    l4_uint32_t flags,
    char* *fname,
    l4dm_dataspace_t *l4env_page,
    CORBA_Environment *_dice_corba_env)
{
  return app_info(task_id, l4env_page, *_dice_corba_obj, fname);
}

/** IDL server loop */
void
server_loop(void)
{
  l4loader_app_server_loop(NULL);
  for (;;) ;
}

/** write error message in buffer to return to client */
int
return_error_msg(int error, const char *msg, const char *fname)
{
  if (error < 0)
    {
      snprintf(error_message, sizeof(error_message),
	      "Error %d (%s) %s", error, l4env_errstr(error), fname);
      error_message[sizeof(error_message)-1] = '\0';
    }

  return error;
}

