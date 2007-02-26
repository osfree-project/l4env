/* $Id$ */
/**
 * \file	loader/server/src/idl.c
 *
 * \date	06/11/2001
 * \author	Frank Mehnert
 *
 * \brief	implemented IDL interface support functions */

#include "idl.h"

#include <stdio.h>

#include "cfg.h"
#include "app.h"

/** Load application
 *
 * \param request	idl request structure
 * \param fname		file name of application
 * \param fprov		id of file provider
 * \param flags		flags (currently unused)
 * \param _ev		flick exception structure
 * \return		0 on success */
l4_int32_t
l4loader_app_server_open(sm_request_t *request, const char *fname, 
			 const l4loader_threadid_t *fprov, l4_uint32_t flags, 
			 sm_exc_t *_ev)
{
  return load_config_script(fname, *(l4_threadid_t*)fprov);
}

/** Kill application
 *
 * \param request	idl request structure
 * \param fname		file name of application to kill
 * \param flags		flags (currently unused)
 * \return		0 on success */
l4_int32_t
l4loader_app_server_kill(sm_request_t *request, 
			 l4_uint32_t task_id, l4_uint32_t flags, sm_exc_t *_ev)
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
l4loader_app_server_dump(sm_request_t *request, 
			 l4_uint32_t task_id, l4_uint32_t flags, sm_exc_t *_ev)
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
l4loader_app_server_info(sm_request_t *request, l4_uint32_t task_id, 
			 l4_uint32_t flags,
			 char **fname, l4loader_dataspace_t *l4env_page,
			 sm_exc_t *_ev)
{
  return app_info(task_id, (l4dm_dataspace_t*)l4env_page, 
		  request->client_tid, fname);
}

/** IDL server loop */
void
server_loop(void)
{
  int ret;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;

  flick_init_request(&request, &ipc_buf);
  for (;;)
    {
      result = flick_server_wait(&request);
      while (!L4_IPC_IS_ERROR(result))
	{
#if DEBUG_REQUEST
	  LOGL("request %08x, src %x.%x\n", ipc_buf.buffer[0],
	      request.client_tid.task, request.client_tid.id.lthread);
#endif
	  switch (ret = l4loader_app_server(&request))
	    {
	    case DISPATCH_ACK_SEND:
	      result = flick_server_reply_and_wait(&request);
	      break;

	    default:
	      printf("Flick dispatch error (%d)!\n", ret);
	      result = flick_server_wait(&request);
	      break;
	    }
	}
      printf("Flick IPC error (%08x)!\n", result.msgdope);
    }
}

