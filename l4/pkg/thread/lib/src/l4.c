/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/l4.c
 * \brief  Architecture dependent functions, native L4 versions
 *
 * \date   04/12/2002
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

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* library includes */
#include "__thread.h"

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Architecture dependent init
 */
/*****************************************************************************/ 
int 
l4th_init_arch(void)
{
  /* nothing to do in native L4 version */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Get pager for new threads
 *	
 * \return Pager thread id
 */
/*****************************************************************************/ 
l4_threadid_t 
l4th_thread_get_pager(void)
{
  /* thread pager -> region mapper thread */
  return l4rm_region_mapper_id();
}

/*****************************************************************************/
/**
 * \brief  Setup thread stack
 * 
 * \param  addr          Stack address 
 * \param  size          Stack size
 * \param  thread        L4 thread id
 * \param  flags         Flags
 *	
 * \return Stack pointer to be used to start new thread
 *
 * Setup thread and return initial stack pointer.
 */
/*****************************************************************************/ 
l4_addr_t
l4th_thread_setup_stack(l4_addr_t addr, l4_size_t size, l4_threadid_t thread,
			l4_uint32_t flags)
{
  int ret;

  /* add new thread to region mapper client list, L4RM needs to know which 
   * threads are allowed to use it */
  if (flags & L4THREAD_CREATE_SETUP)
    ret = l4rm_direct_add_client(thread);
  else
    ret = l4rm_add_client(thread);
  if (ret < 0)
    {
      /* failed, ignore error since it does not mean that the thread 
       * cannot run. */
      ERROR("l4thread: add thread to region mapper clients failed: %s (%d)!",
	    l4env_errstr(ret),ret);
    }

  /* nothing else necessary, return stack pointer */
  return addr + size;
}

/*****************************************************************************/
/**
 * \brief  Entry point for new threads
 */
/*****************************************************************************/ 
void
l4th_thread_entry(void)
{
  /* nothing special to do in native L4 version, just start the thread */
  l4th_thread_start();
}
