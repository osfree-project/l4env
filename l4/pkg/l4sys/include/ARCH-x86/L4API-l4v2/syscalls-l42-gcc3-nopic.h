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
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(fpage_unmap)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=a" (dummy1),
	  "=c" (dummy2)
	 :
	  "a" (fpage),
	  "c" (map_mask)
	 :
	  "ebx", "edx", "edi", "esi"
	 );
}

/*
 * L4 id myself
 */
L4_INLINE l4_threadid_t
l4_myself(void)
{
  l4_threadid_t temp_id;

  __asm__(
	  "push	%%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(id_nearest)
	  "popl	%%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  :
	   "=S" (temp_id.lh.low),
	   "=D" (temp_id.lh.high)
	  :
	   "S" (0)
	  :
	   "ebx", "eax", "ecx", "edx"
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
  int type;
  __asm__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(id_nearest)
	  "popl	%%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  :
	   "=S" (next_chief->lh.low),
	   "=D" (next_chief->lh.high),
	   "=a" (type)
	  :
	   "S" (destination.lh.low),
	   "D" (destination.lh.high)
	  :
	   "ebx", "ecx", "edx"
	  );
  return type;
}

/*
 * L4 lthread_ex_regs
 */
L4_INLINE void
__do_l4_thread_ex_regs(l4_umword_t val0,
                       l4_umword_t eip,
                       l4_umword_t esp,
                       l4_threadid_t *preempter,
                       l4_threadid_t *pager,
                       l4_umword_t *old_eflags,
                       l4_umword_t *old_eip,
                       l4_umword_t *old_esp)
{
  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory  references
					   ("m") after this point */
	  "pushl %%ebx		\n\t"	/* save address of preempter */
	  "movl	4(%%ebx), %%ebp	\n\t"	/* load new preempter id */
	  "movl	 (%%ebx), %%ebx	\n\t"

	  L4_SYSCALL(lthread_ex_regs)

	  "xchgl (%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
					   and get address of preempter */
	  "movl	%%ebp, 4(%%ebx)	\n\t"	/* write preempter.lh.high */
	  "popl	(%%ebx)		\n\t"	/* write preempter.lh.low */
	  "popl	%%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  :
	   "=a" (*old_eflags),
	   "=c" (*old_esp),
	   "=d" (*old_eip),
	   "=S" (pager->lh.low),
	   "=D" (pager->lh.high)
	  :
	   "a" (val0),
	   "c" (esp),
	   "d" (eip),
	   "S" (pager->lh.low),
	   "D" (pager->lh.high),
	   "b" (preempter)
	  :
	   "memory"
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
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(thread_switch)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=S" (dummy),
	  "=a" (dummy)
	 :
	  "S" (destination.lh.low),
	  "a" (0)			/* Fiasco requirement */
	 :
	  "ebx", "ecx", "edx", "edi"
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
  l4_uint32_t time_lo, time_hi;
  unsigned dummy;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "pushl %%ecx		\n\t"	/* save address of preempter */
	  "movl  (%%ecx), %%ebx	\n\t"	/* load preempter id.low */
	  "movl 4(%%ecx), %%ebp	\n\t"	/* load preempter id.high */
	  "cmpl $-1,%%eax	\n\t"
	  "jz   1f		\n\t"	/* don't change if invalid */
	  "andl $0xfff0ffff, %%eax\n\t"	/* mask bits that must be zero */
	  "1:			\n\t"
	  L4_SYSCALL(thread_schedule)
	  "xchgl (%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
					   and get address of preempter */
	  "popl	 (%%ebx)	\n\t"	/* write preempter.lh.low */
	  "movl	 %%ebp, 4(%%ebx)\n\t"	/* write preempter.lh.high */
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=a" (*old_param),
	  "=c" (time_lo),
	  "=d" (time_hi),
	  "=S" (partner->lh.low),
	  "=D" (partner->lh.high),
	  "=b" (dummy)
	 :
	  "a" (param),
	  "S" (dest.lh.low),
	  "D" (dest.lh.high),
	  "c" (ext_preempter)
	 :
	  "memory"
	 );
  return (((l4_cpu_time_t)time_hi) << 32) | time_lo;
}

/*
 * L4 task new
 */
L4_INLINE l4_taskid_t
__do_l4_task_new(l4_taskid_t destination,
	         l4_umword_t mcp_or_new_chief_and_flags,
	         l4_umword_t esp,
	         l4_umword_t eip,
	         l4_threadid_t pager)
{
  unsigned dummy1, dummy2, dummy3, dummy4;
  l4_taskid_t new_task;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  4(%%ebx), %%ebp\n\t"	/* load pager id */
	  "movl   (%%ebx), %%ebx\n\t"
	  L4_SYSCALL(task_new)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=b" (dummy1),
	  "=a" (dummy2),
	  "=c" (dummy3),
	  "=d" (dummy4),
	  "=S" (new_task.lh.low),
	  "=D" (new_task.lh.high)
	 :
	  "b" (&pager),
	  "a" (mcp_or_new_chief_and_flags),
	  "c" (esp),
	  "d" (eip),
	  "S" (destination.lh.low),
	  "D" (destination.lh.high),
	  "m" (pager) // make the compiler to know that we dereference the
                      // &pager value
	 );
  return new_task;
}

/*
 * L4 task new
 */
L4_INLINE l4_taskid_t
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief,
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager)
{
  return __do_l4_task_new(destination, mcp_or_new_chief, esp, eip, pager);
}

/*
 * L4 task new with cap
 */
L4_INLINE l4_taskid_t
l4_task_new_cap(l4_taskid_t destination,
	        l4_umword_t mcp_or_new_chief,
	        l4_umword_t esp,
	        l4_umword_t eip,
	        l4_threadid_t pager,
	        l4_threadid_t cap_handler)
{
  l4_utcb_get()->task_new.caphandler = cap_handler;
  l4_utcb_get()->task_new.quota.raw = 0;
  return __do_l4_task_new(destination,
                          mcp_or_new_chief | L4_TASK_NEW_IPC_MONITOR,
                          esp, eip, pager);
}

L4_INLINE int
l4_privctrl(l4_umword_t cmd,
	    l4_umword_t param)
{
  int err;
  unsigned dummy;

  __asm__ __volatile__(
	 "pushl %%ebp              \n\t"
	 L4_SYSCALL(privctrl)
         "popl  %%ebp              \n\t"
	 :"=a"(err), "=d"(dummy)
	 :"0"(cmd),"1"(param));
  return err;
}

#endif
