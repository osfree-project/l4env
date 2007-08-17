/*
 * $Id$
 */

#ifndef __L4_SYSCALLS_H__
#define __L4_SYSCALLS_H__

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>
#include <l4/sys/types.h>
#include <l4/sys/consts.h>

/*
 * prototypes
 */
L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask);

L4_INLINE l4_threadid_t
l4_myself(void);

L4_INLINE l4_threadid_t
l4_myself_noprof(void) L4_NOINSTRUMENT;

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
l4_thread_ex_regs_flags(l4_threadid_t destination,
                        l4_umword_t ip,
                        l4_umword_t sp,
                        l4_threadid_t *preempter,
                        l4_threadid_t *pager,
                        l4_umword_t *old_cpsr,
                        l4_umword_t *old_ip,
                        l4_umword_t *old_sp,
                        unsigned long flags);

L4_INLINE void
l4_inter_task_ex_regs(l4_threadid_t destination,
                      l4_umword_t ip,
                      l4_umword_t sp,
                      l4_threadid_t *preempter,
                      l4_threadid_t *pager,
                      l4_umword_t *old_cpsr,
                      l4_umword_t *old_ip,
                      l4_umword_t *old_sp,
                      unsigned long flags);

L4_INLINE l4_threadid_t
l4_thread_ex_regs_pager(l4_threadid_t destination);

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
	    l4_umword_t mcp_or_new_chief_and_flags,
	    l4_umword_t sp,
	    l4_umword_t ip,
	    l4_threadid_t pager);

L4_INLINE l4_taskid_t
l4_task_new_long(l4_taskid_t destination,
                 l4_umword_t mcp_or_new_chief_and_flags,
                 l4_umword_t esp,
                 l4_umword_t eip,
                 l4_threadid_t pager,
                 l4_threadid_t cap_handler,
                 l4_quota_desc_t kquota);

L4_INLINE void
l4_yield (void);

L4_INLINE
void *l4_kernel_interface(void);

L4_INLINE int
l4_privctrl(l4_umword_t cmd,
            l4_umword_t param);

/**
 * Internal prototypes
 */
L4_INLINE void
__do_l4_thread_ex_regs(l4_umword_t val0,
                       l4_umword_t ip,
                       l4_umword_t sp,
                       l4_threadid_t *preempter,
                       l4_threadid_t *pager,
                       l4_umword_t *old_cpsr,
                       l4_umword_t *old_ip,
                       l4_umword_t *old_sp);

L4_INLINE l4_taskid_t
__do_l4_task_new(l4_taskid_t destination,
                 l4_umword_t mcp_or_new_chief_and_flags,
                 l4_umword_t sp,
                 l4_umword_t ip,
                 l4_threadid_t pager);

/*----------------------------------------------------------------------------
 * Implementation
 *--------------------------------------------------------------------------*/

#include <l4/sys/syscalls_impl.h>

/* --- l4_thread_ex_regs variants ----- */

L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination,
                  l4_umword_t ip,
                  l4_umword_t sp,
                  l4_threadid_t *preempter,
                  l4_threadid_t *pager,
                  l4_umword_t *old_cpsr,
                  l4_umword_t *old_ip,
                  l4_umword_t *old_sp)
{
  __do_l4_thread_ex_regs(destination.id.lthread,
                         ip, sp, preempter, pager,
                         old_cpsr, old_ip, old_sp);
}

L4_INLINE void
l4_thread_ex_regs_flags(l4_threadid_t destination,
                        l4_umword_t ip,
                        l4_umword_t sp,
                        l4_threadid_t *preempter,
                        l4_threadid_t *pager,
                        l4_umword_t *old_cpsr,
                        l4_umword_t *old_ip,
                        l4_umword_t *old_sp,
                        unsigned long flags)
{
  __do_l4_thread_ex_regs(destination.id.lthread | flags,
                         ip, sp, preempter, pager,
                         old_cpsr, old_ip, old_sp);
}

L4_INLINE void
l4_inter_task_ex_regs(l4_threadid_t destination,
                      l4_umword_t ip,
                      l4_umword_t sp,
                      l4_threadid_t *preempter,
                      l4_threadid_t *pager,
                      l4_umword_t *old_cpsr,
                      l4_umword_t *old_ip,
                      l4_umword_t *old_sp,
                      unsigned long flags)
{
  __do_l4_thread_ex_regs(destination.id.lthread
                          | (destination.id.task << 7) | flags,
                         ip, sp, preempter, pager,
                         old_cpsr, old_ip, old_sp);
}

L4_INLINE l4_threadid_t
l4_thread_ex_regs_pager(l4_threadid_t destination)
{
  l4_umword_t dummy;
  l4_threadid_t preempter = L4_INVALID_ID;
  l4_threadid_t pager     = L4_INVALID_ID;

  l4_thread_ex_regs_flags(destination, (l4_umword_t)-1, (l4_umword_t)-1,
                          &preempter, &pager, &dummy, &dummy, &dummy,
                          L4_THREAD_EX_REGS_NO_CANCEL);
  return pager;
}

/* --- l4_task_new variants ----- */

L4_INLINE l4_taskid_t
l4_task_new(l4_taskid_t destination,
            l4_umword_t mcp_or_new_chief_and_flags,
            l4_umword_t esp,
            l4_umword_t eip,
            l4_threadid_t pager)
{
  return __do_l4_task_new(destination, mcp_or_new_chief_and_flags,
                          esp, eip, pager);
}

L4_INLINE l4_taskid_t
l4_task_new_long(l4_taskid_t destination,
                 l4_umword_t mcp_or_new_chief_and_flags,
                 l4_umword_t esp,
                 l4_umword_t eip,
                 l4_threadid_t pager,
                 l4_threadid_t cap_handler,
                 l4_quota_desc_t kquota)
{
  l4_utcb_get()->task_new.caphandler = cap_handler;
  l4_utcb_get()->task_new.quota = kquota;
  return __do_l4_task_new(destination,
                          mcp_or_new_chief_and_flags | L4_TASK_NEW_UTCB_ARGS,
                          esp, eip, pager);
}

/* ---- */


L4_INLINE void
l4_yield(void)
{
  l4_thread_switch(L4_NIL_ID);
}

#endif /* ! __L4_SYSCALLS_H__ */
