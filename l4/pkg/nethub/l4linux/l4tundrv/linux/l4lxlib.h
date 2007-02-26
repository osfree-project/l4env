/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#ifndef L4_LX_LIB_H__
#define L4_LX_LIB_H__

#include <l4/thread/thread.h>
#include <l4/dde_linux/dde.h>
#include <l4/sys/types.h>
#include <assert.h>


inline
l4_threadid_t
l4lx_thread_create(void (*func)(void *), void *stack_ptr, 
                   void *data, unsigned stack_data_size,
		   int prio, const char *name)
{
  l4_threadid_t ret;
  l4thread_t thread_no;

  assert(stack_ptr == NULL);
  assert(data == NULL || stack_data_size == sizeof(void*));

  if ((thread_no = l4thread_create_long
	(L4THREAD_INVALID_ID,
	 func, name, L4THREAD_INVALID_SP, 1024,
	 (prio == -1) ? L4THREAD_DEFAULT_PRIO : prio,
	 data,
	 L4THREAD_CREATE_ASYNC)) < 0 ) 
    {
      printk("%s: Error creating thread %x.%x,%d\n",
	     __func__, ret.id.task, ret.id.lthread,
 	     thread_no);
      enter_kdebug("L4thread error creating thread!");
  }

  ret = l4thread_l4_id(thread_no);

  printk("%s: Created thread %x.%x (%s)\n",
         __func__, ret.id.task, ret.id.lthread, name);

  return ret;
}

inline
void 
l4lx_thread_shutdown(l4_threadid_t thread)
{ l4thread_shutdown(l4thread_id(thread)); }


#endif // L4_LX_LIB_H__
