/*
 * $Id$
 */

#ifndef __L4_SYSCALLS_L42_GCC3_H__
#define __L4_SYSCALLS_L42_GCC3_H__

/*
 * L4 flex page unmap
 */
L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask)
{
  l4_umword_t dummy1, dummy2, dummy3;

  __asm__ __volatile__(
	  L4_SYSCALL(fpage_unmap)
	 :
	  "=a" (dummy1),
	  "=b" (dummy2),
	  "=c" (dummy3)
	 :
	  "a" (fpage),
	  "c" (map_mask)
	 :
	  "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", 
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
  l4_umword_t dummy;

  __asm__ __volatile__(
	  L4_SYSCALL(id_nearest)
	  :
	   "=S" (temp_id),
	   "=b" (dummy)
	  :
	   "S" (0)
	  :
	   "rax", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", 
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
  l4_umword_t dummy;
  __asm__ (
	  L4_SYSCALL(id_nearest)
	  :
	   "=S" (*next_chief),
	   "=a" (type),
	   "=b" (dummy)
	  :
	   "S" (destination)
	  :
	   "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", 
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
  register l4_umword_t r8 asm("r8") = preempter->raw;
  l4_umword_t dummy;
  __asm__ __volatile__ (
	  L4_SYSCALL(lthread_ex_regs)
	  :
	   "=a" (*old_rflags),
	   "=b" (dummy),
	   "=c" (*old_rsp),
	   "=d" (*old_rip),
	   "=S" (*pager),
	   "=r" (r8)
	  :
	   "a" (val0),
	   "c" (rsp),
	   "d" (rip),
	   "S" (*pager),
	   "r" (r8)
	  :
	  "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory"
	  );
  preempter->raw = r8;
}

/*
 * L4 thread switch
 */
L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
  l4_umword_t dummy1, dummy2, dummy3;
  __asm__ __volatile__(
	  L4_SYSCALL(thread_switch)
	 :
	  "=S" (dummy1),
	  "=a" (dummy2),
	  "=b" (dummy3)
	 :
	  "S" (destination.raw),
	  "a" (0)			/* Fiasco requirement */
	 :
	  "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11", "r12", 
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

  register l4_umword_t r8 asm ("r8") = ext_preempter->raw;
  l4_umword_t dummy;

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
	  "=b" (dummy),
	  "=c" (time),
	  "=S" (*partner),
	  "=r" (r8)
	 :
	  "a" (param),
	  "S" (dest),
	  "r" (r8)
	 :
	  "rdi", "rdx", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
	 );
  ext_preempter->raw = r8;
  return time;
}

/*
 * L4 task new
 */
L4_INLINE l4_taskid_t
__do_l4_task_new(l4_taskid_t destination,
                 l4_umword_t mcp_or_new_chief_and_flags,
                 l4_umword_t rsp,
                 l4_umword_t rip,
                 l4_threadid_t pager)
{
  l4_umword_t dummy2, dummy3, dummy4, dummy5;
  l4_taskid_t new_task;

  register l4_umword_t r8 asm ("r8") = pager.raw;

  __asm__ __volatile__(
	  L4_SYSCALL(task_new)
	 :
	  "=r" (r8),
	  "=a" (dummy2),
	  "=b" (dummy3),
	  "=c" (dummy4),
	  "=d" (dummy5),
	  "=S" (new_task)
	 :
	  "r" (r8),
	  "a" (mcp_or_new_chief_and_flags),
	  "c" (rsp),
	  "d" (rip),
	  "S" (destination)
	 : "rdi", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
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
  l4_umword_t dummy1, dummy2;

  __asm__ __volatile__(
	 L4_SYSCALL(privctrl)
	 :
	  "=a"(err),
	  "=b"(dummy1),
	  "=d"(dummy2)
	 :
	  "a"(cmd),
	  "d"(param));
  return err;
}

#endif
