/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/libl4rm.c
 * \brief  Region mapper library.
 *
 * \date   06/01/2000
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
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>

/* private includes */
#include "l4rm-server.h"
#include "__libl4rm.h"
#include "__alloc.h"
#include "__region.h"
#include "__avl_tree.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Region mapper lib initialized
 */
static int l4rm_initialized = 0;

/**
 * region mapper service thread
 */
l4_threadid_t l4rm_service_id = L4_INVALID_ID;

/**
 * Region mapper stack, these symbols must be definied in __crt0.S
 */
extern unsigned long stack_low;
extern unsigned long stack_high;

/**
 * Pager Id, used to forward unknown pagefaults (if enabled)
 */
l4_threadid_t l4rm_task_pager_id;

/* Test if request is a pagefault, the Flick message encoding differs in 
 * L4 API v2 and x0 */
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
#  define IS_PAGEFAULT(req) (req.msgbuf->buffer[0] == 0)
#  define PF_EIP(req)       (req.msgbuf->buffer[2])
#  define PF_ADDR(req)      (req.msgbuf->buffer[1])
#else
#  define IS_PAGEFAULT(req) (req.msgbuf->buffer[0] < L4RM_FIRST_REQUEST) \
			    || l4_is_io_page_fault(req.msgbuf->buffer[0])
#  define PF_EIP(req)       (req.msgbuf->buffer[1])
#  define PF_ADDR(req)      (req.msgbuf->buffer[0])
#endif

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Determine task pager id
 */
/*****************************************************************************/ 
static void
__get_pager(void)
{
  l4_umword_t dummy;
  l4_threadid_t l4rm_task_preempter_id;
  
  l4rm_task_preempter_id = l4rm_task_pager_id = L4_INVALID_ID;
  l4_thread_ex_regs(l4rm_service_id, (l4_umword_t)-1, (l4_umword_t)-1,
		    &l4rm_task_preempter_id, &l4rm_task_pager_id,
		    &dummy, &dummy, &dummy);
}

/*****************************************************************************
 *** L4RM client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Setup region mapper lib. 
 *
 * \param  have_l4env    Set to != 0 if started with the L4 environment 
 *                       (L4RM then uses the default dataspace manager to 
 *                       allocate its descriptor heap)
 * \param  used          Used VM address range, do not use for internal data.
 * \param  num_used      Number of elements in \a used.
 * 
 * \retval 0 on success, -1 if initialization failed 
 */
/*****************************************************************************/ 
int
l4rm_init(int have_l4env, 
	  l4rm_vm_range_t used[], 
	  int num_used)
{
  int ret;

  if (cmpxchg32(&l4rm_initialized,0,1))
    {
      /* service thread id */
      l4rm_service_id = l4_myself();

      /* get my pager id */
      __get_pager();

      /* setup descriptor heap */
      ret = l4rm_heap_init(have_l4env,used,num_used);
      if (ret < 0)
	{
	  Panic("L4RM: heap init failed!");
	  return -1;
	}

      /* region table */
      ret = l4rm_init_regions();
      if (ret < 0)
	{
	  Panic("L4RM: setup region list failed!");
	  return -1;
	}
      
      /* region tree */
      ret = avlt_init();
      if (ret < 0)
	{
	  Panic("L4RM: set region search tree failed!");
	  return -1;
	}

      /* register descriptor heap */
      ret = l4rm_heap_register();
      if (ret < 0)
	{
	  Panic("L4RM: register heap failed!");
	  return -1;
	}
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Return service id of region mapper thread
 *	
 * \return Id of region mapper thread.
 */
/*****************************************************************************/ 
l4_threadid_t
l4rm_region_mapper_id(void)
{
  /* return id */
  return l4rm_service_id;
}

/*****************************************************************************/
/**
 * \brief Region Mapper service loop.
 * 
 * Region mapper service loop. Currently, we must distinguish between Flick
 * requests and pagefaults by hand (all messages with dw0 >= 0xC0000000
 * are considered to be Flick requests, all other requests are pagefaults).
 * This should be handled by the IDL compiler at some stage.
 */
/*****************************************************************************/ 
void
l4rm_service_loop(void)
{
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;
  int ret;

  /* init flick messages */
  flick_init_request(&request, &ipc_buf);

  /* service loop */
  while(1)
    {
      result = flick_server_wait(&request);

      while (!L4_IPC_IS_ERROR(result))
	{
#if DEBUG_REQUEST
	  INFO("request 0x%08x, src %x.%x\n",ipc_buf.buffer[0],
	       request.client_tid.id.task,request.client_tid.id.lthread);
#endif
	  /* we only accept message from our own task */	 
	  if (!l4_task_equal(request.client_tid,l4rm_service_id))
	    {
	      Msg("L4RM: blocked message from outside (%x.%x)!\n",
		  request.client_tid.id.task,request.client_tid.id.lthread);
	      break;
	    }

	  /* UGLY: first check if message is a pagefault */
	  if (IS_PAGEFAULT(request))
	    {
	      /* handle pagefault */
	      l4rm_handle_pagefault(request.client_tid,PF_ADDR(request),
				    PF_EIP(request));

	      /* setup reply message, pages are mapped in
	       * l4rm_handle_pagefault, so the reply is empty */
	      request.msgbuf->sndmsg = L4_IPC_SHORT_MSG;

	      /* reply and wait for next request */
	      result = flick_server_reply_and_wait(&request);
	    }
	  else
	    {
#if DEBUG_REQUEST
	      DMSG("  Flick request...\n");
#endif
	      /* dispatch request */
	      ret = l4_rm_server(&request);
	      switch(ret)
		{
		case DISPATCH_ACK_SEND:
		  /* reply and wait for next request */
		  result = flick_server_reply_and_wait(&request);
		  break;

		default:
		  Msg("L4RM: Flick dispatch error (%d)!\n",ret);
		  
		  /* wait for next request */
		  result = flick_server_wait(&request);
		  break;
		}
	    }
	}

      Panic("L4RM: Flick IPC error (0x%08x)!",result.msgdope);       
    }

  /* this should never happen */
  Panic("left L4RM service loop");
}


