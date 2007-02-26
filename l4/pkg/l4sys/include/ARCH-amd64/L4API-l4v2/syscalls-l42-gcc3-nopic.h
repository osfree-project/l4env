/*
 * $Id$
 */

#ifndef __L4_SYSCALLS_L42_GCC3_NOPIC_H__
#define __L4_SYSCALLS_L42_GCC3_NOPIC_H__

/*
 * L4 flex page unmap
 */
L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask)
{
  unsigned dummy1, dummy2;

  __asm__ __volatile__(
	  L4_SYSCALL(fpage_unmap)
	 :
	  "=a" (dummy1),
	  "=c" (dummy2)
	 :
	  "a" (fpage),
	  "c" (map_mask)
	 :
	  "rbx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", 
	  "r13", "r14", "r15"
	 );
};

/*
 * L4 id myself
 */
L4_INLINE l4_threadid_t
l4_myself(void)
{
  l4_threadid_t temp_id;

  __asm__ __volatile__(
	  L4_SYSCALL(id_nearest)
	  :
	   "=S" (temp_id)
	  :
	   "S" (0)
	  :
	   "rbx", "rax", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", 
	   "r13", "r14", "r15"
	  );
  return temp_id;
}

/*
 * L4 id next chief
 */
L4_INLINE int
l4_nchief(l4_threadid_t destination,
	  l4_threadid_t *next_chief)
{
  l4_umword_t type;
  __asm__ (
	  L4_SYSCALL(id_nearest)
	  :
	   "=S" (*next_chief),
	   "=a" (type)
	  :
	   "S" (destination)
	  :
	   "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", 
	   "r13", "r14", "r15"
	  );
  return type;
}

/*
 * L4 lthread_ex_regs
 */
L4_INLINE void
__do_l4_thread_ex_regs(l4_umword_t val0,
                       l4_umword_t rip,
		       l4_umword_t rsp,
		       l4_threadid_t *preempter,
		       l4_threadid_t *pager,
		       l4_umword_t *old_rflags,
		       l4_umword_t *old_rip,
		       l4_umword_t *old_rsp)
{
  __asm__ __volatile__ (
	  L4_SYSCALL(lthread_ex_regs)
	  :
	   "=a" (*old_rflags),
	   "=c" (*old_rsp),
	   "=d" (*old_rip),
	   "=S" (*pager),
	   "=b" (*preempter)
	  :
	   "a" (val0),
	   "c" (rsp),
	   "d" (rip),
	   "S" (*pager),
	   "b" (*preempter)
	  :
	  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory"
	  );
}

/*
 * L4 thread switch
 */
L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
  long dummy;
  __asm__ __volatile__(
	  L4_SYSCALL(thread_switch)
	 :
	  "=S" (dummy),
	  "=a" (dummy)
	 :
	  "S" (destination.lh.low),
	  "a" (0)			/* Fiasco requirement */
	 :
	  "rbx", "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11", "r12", 
	  "r13", "r14", "r15"
	 );
}

/*
 * L4 thread schedule
 */
L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param)
{
  l4_cpu_time_t time;

  __asm__ __volatile__(
	  "mov $~0, %%rcx\n\t"  // XXX workaround for cmp signed ext
	  "cmp %%rax,%%rcx	\n\t"
	  "jz   1f		\n\t"	/* don't change if invalid */
	  "mov $0xfff0ffff, %%rcx\n\t"	/* mask bits that must be zero */
	  "and %%rcx,%%rax	\n\t"   // XXX workaround for signed extension
	  "1:			\n\t"
	  L4_SYSCALL(thread_schedule)
	 :
	  "=a" (*old_param),
	  "=c" (time),
	  "=S" (*partner),
	  "=b" (*ext_preempter)
	 :
	  "a" (param),
	  "S" (dest),
	  "b" (*ext_preempter)
	 :
	  "rdi", "rdx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
	 );
  return time;
}

/*
 * L4 task new
 */
L4_INLINE l4_taskid_t
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief,
	    l4_umword_t rsp,
	    l4_umword_t rip,
	    l4_threadid_t pager)
{
  unsigned dummy1, dummy2, dummy3, dummy4;
  l4_taskid_t new_task;

  __asm__ __volatile__(
	  L4_SYSCALL(task_new)
	 :
	  "=b" (dummy1),
	  "=a" (dummy2),
	  "=c" (dummy3),
	  "=d" (dummy4),
	  "=S" (new_task)
	 :
	  "b" (pager),
	  "a" (mcp_or_new_chief),
	  "c" (rsp),
	  "d" (rip),
	  "S" (destination)
	 : "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
	 );
  return new_task;
}

/*
 * L4 privilege control
 */
L4_INLINE int
l4_privctrl(l4_umword_t cmd,
	    l4_umword_t param)
{
  int err;
  unsigned dummy;

  __asm__ __volatile__(
	 L4_SYSCALL(privctrl)
	 :
	  "=a"(err),
	  "=d"(dummy)
	 :
	  "a"(cmd),
	  "d"(param));
  return err;
}

#endif

