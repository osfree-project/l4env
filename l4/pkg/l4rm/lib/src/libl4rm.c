/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/libl4rm.c
 * \brief  Region mapper library.
 *
 * \date   06/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
#include "l4/l4rm/l4rm-server.h"
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

  if (l4util_cmpxchg32(&l4rm_initialized,0,1))
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
 * Region mapper service loop. The Dice server loop distinguishes between
 * client requests (attach, ...) and pagefaults. It calls the appropriate 
 * functions resp. the pagefault handler. 
 */
/*****************************************************************************/ 
void
l4rm_service_loop(void)
{
  /* Dice server loop */
  l4_rm_server_loop(NULL);

  /* this should never happen */
  Panic("left L4RM service loop");
}


