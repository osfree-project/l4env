/*
 * $Id$
 */

#ifndef __L4_SYSCALLS_L42_GCC295_NOPIC_PROFILE_H__
#define __L4_SYSCALLS_L42_GCC295_NOPIC_PROFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void l4_fpage_unmap_static(l4_fpage_t fpage,
				  l4_umword_t map_mask);

extern l4_threadid_t l4_myself_static(void);

extern int  l4_nchief_static(l4_threadid_t destination,
			     l4_threadid_t *next_chief);

extern void l4_thread_ex_regs_static(l4_threadid_t destination,
				     l4_umword_t eip,
				     l4_umword_t esp,
				     l4_threadid_t *preempter,
				     l4_threadid_t *pager,
				     l4_umword_t *old_eflags,
				     l4_umword_t *old_eip,
				     l4_umword_t *old_esp);

extern void l4_thread_switch_static(l4_threadid_t destination);

extern l4_cpu_time_t l4_thread_schedule_static(l4_threadid_t dest,
					       l4_sched_param_t param,
					       l4_threadid_t *ext_preempter,
					       l4_threadid_t *partner,
					       l4_sched_param_t *old_param);

extern l4_taskid_t l4_task_new_static(l4_taskid_t destination,
				      l4_umword_t mcp_or_new_chief,
				      l4_umword_t esp,
				      l4_umword_t eip,
				      l4_threadid_t pager);

#ifdef __cplusplus
}
#endif


L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask)
{
  l4_fpage_unmap_static(fpage, map_mask);
}

L4_INLINE l4_threadid_t
l4_myself(void)
{
  return l4_myself_static();
}

L4_INLINE l4_threadid_t
l4_myself_noprof(void)
{
  l4_threadid_t temp_id;

  __asm__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(id_nearest)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  :
	   "=S" (temp_id.lh.low),	/* ESI, 0 */
	   "=D" (temp_id.lh.high)	/* EDI, 1 */
	  :
	   "0" (0)			/* ESI, nil id (id.low = 0) */
	  :
	   "eax", "ecx", "edx"
	  );
  return temp_id;
}

L4_INLINE int
l4_nchief(l4_threadid_t destination,
	  l4_threadid_t *next_chief)
{
  return l4_nchief_static(destination, next_chief);
}

L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination,
		  l4_umword_t eip,
		  l4_umword_t esp,
		  l4_threadid_t *preempter,
		  l4_threadid_t *pager,
		  l4_umword_t *old_eflags,
		  l4_umword_t *old_eip,
		  l4_umword_t *old_esp)
{
  l4_thread_ex_regs_static(destination, eip, esp, preempter, pager,
                    old_eflags, old_eip, old_esp);
}

L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
  l4_thread_switch_static(destination);
}

L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param)
{
  return l4_thread_schedule_static(dest, param, ext_preempter, partner,
				   old_param);
}

L4_INLINE l4_taskid_t
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief,
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager)
{
  return l4_task_new_static(destination, mcp_or_new_chief, esp, eip, pager);
}

#endif

