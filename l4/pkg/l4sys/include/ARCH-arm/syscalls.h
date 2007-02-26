/* 
 * $Id$
 */

#ifndef __L4_SYSCALLS_H__ 
#define __L4_SYSCALLS_H__ 

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>
#include <l4/sys/types.h>
#include <l4/sys/consts_arch.h>

/*
 * prototypes
 */
L4_INLINE void 
l4_fpage_unmap(l4_fpage_t fpage, 
	       l4_umword_t map_mask);

L4_INLINE l4_threadid_t 
l4_myself(void);

L4_INLINE int 
l4_nchief(l4_threadid_t destination, 
	  l4_threadid_t *next_chief);

L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination, 
		  l4_umword_t ip, 
		  l4_umword_t sp,
		  l4_threadid_t *preempter, 
		  l4_threadid_t *pager,
		  l4_umword_t *old_cpsr, 
		  l4_umword_t *old_ip, 
		  l4_umword_t *old_sp);
L4_INLINE void
l4_thread_switch(l4_threadid_t destination);

L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest, 
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter, 
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param);

L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination, 
	    l4_umword_t mcp, 
	    l4_umword_t sp, 
	    l4_umword_t ip, 
	    l4_threadid_t pager);

L4_INLINE void
l4_yield (void);

L4_INLINE 
void *l4_kernel_interface(void);

/*----------------------------------------------------------------------------
 * Implementation
 *--------------------------------------------------------------------------*/

#include <l4/sys/syscalls_impl.h>

L4_INLINE void
l4_yield ()
{
  l4_thread_switch(L4_NIL_ID);
}

#endif

