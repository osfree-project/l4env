/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/server.c
 * \brief  DMphys server function
 * 
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include "__dm_phys.h"
#include "__debug.h"

/*****************************************************************************/
/**
 * \brief DMphys Flick server function
 *
 * We do not need any indirect buffers, everything is transfered in the 
 * flick message buffer.
 */
/*****************************************************************************/ 
void
dmphys_server(void)
{
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;
  int ret;

  /* Flick server loop */
  flick_init_request(&request, &ipc_buf);
  while (1)
    {
      result = flick_server_wait(&request);

      while (!L4_IPC_IS_ERROR(result))
	{
#if DEBUG_FLICK_REQUEST
	  INFO("request 0x%08x, src %x.%x\n",ipc_buf.buffer[0],
	       request.client_tid.id.task,request.client_tid.id.lthread);
#endif
	  /* dispatch request */
	  ret = if_l4dm_memphys_server(&request);
	  switch (ret)
	    {
	      case DISPATCH_ACK_SEND:
	      /* reply and wait for next request */
	      result = flick_server_reply_and_wait(&request);
	      break;
	      
	    default:
	      Error("Flick dispatch error (%d)!\n",ret);
	      
	      /* wait for next request */
	      result = flick_server_wait(&request);
	      break;
	    }
	}
      Error("DMphys: Flick IPC error (0x%08x)!\n",result.msgdope);
    }
  
  /* this should never happen */
  return;
}
