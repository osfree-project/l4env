/*
 * $Id$
 */

#ifndef __L4_SYSCALLS_L42_GCC3_PIC_H__
#define __L4_SYSCALLS_L42_GCC3_PIC_H__

/*
 * L4 flex page unmap
 */
L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask)
{
  unsigned dummy1, dummy2;

  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(fpage_unmap)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx  	\n\t"

	 :
	  "=a" (dummy1),
	  "=c" (dummy2)
	 :
	  "a" (fpage),
	  "c" (map_mask)
	 :
	  "edx", "edi", "esi"
	 );
}

/*
 * L4 id myself
 */
L4_INLINE l4_threadid_t
l4_myself(void)
{
  l4_threadid_t temp_id;

  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(id_nearest)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  :
	   "=S" (temp_id.raw)		/* ESI, ID */
	  :
	   "S" (0)			/* ESI, nil id */
	  :
	   "eax", "ecx", "edx", "edi"
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
  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(id_nearest)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  :
	   "=S" (next_chief->raw),	/* ESI, 0 */
	   "=a" (type)			/* EAX, 2 */
	  :
	   "S" (destination.raw)	/* ESI */
	  :
	   "ecx", "edx", "edi"
	  );
  return type;
}

/*
 * L4 lthread_ex_regs
 */
L4_INLINE void
l4_thread_ex_regs_sc(l4_umword_t val0,
                     l4_umword_t eip,
                     l4_umword_t esp,
                     l4_threadid_t *preempter,
                     l4_threadid_t *pager,
                     l4_umword_t *old_eflags,
                     l4_umword_t *old_eip,
                     l4_umword_t *old_esp)
{
  __asm__ __volatile__(
	  "pushl %%ebx  	\n\t"
	  "movl  %%edi, %%ebx	\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(lthread_ex_regs)

	  "movl	 %%ebx, %%edi   \n\t"	/* write preempter.lh.high */
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  :
	  "=a" (*old_eflags),
	  "=c" (*old_esp),
	  "=d" (*old_eip),
	  "=S" (pager->raw),
	  "=D" (preempter->raw)
	  :
	  "a" (val0),
	  "c" (esp),
	  "d" (eip),
	  "S" (pager->raw),
	  "D" (preempter->raw)
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
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(thread_switch)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	 :
	  "=S" (dummy),
	  "=a" (dummy)
	 :
	  "S" (destination.raw),
	  "a" (0)			/* Fiasco requirement */
	 :
	  "ecx", "edx", "edi"
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

  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  %%edi, %%ebx	\n\t"	/* load preempter id.low */
	  "cmpl $-1,%%eax	\n\t"
	  "jz   1f		\n\t"	/* don't change if invalid */
	  "andl $0xfff0ffff, %%eax\n\t"	/* mask bits that must be zero */
	  "1:			\n\t"
	  L4_SYSCALL(thread_schedule)
	  "movl  %%ebx, %%edi	\n\t"	/* write preempter.lh.low */
	  "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"

	 :
	  "=a" (*old_param),
	  "=c" (time_lo),
	  "=d" (time_hi),
	  "=S" (partner->raw),
	  "=D" (ext_preempter->raw)
	 :
	  "a" (param),
	  "S" (dest.raw),
	  "D" (ext_preempter->raw)
	 :
	  "memory");
  return (((l4_cpu_time_t)time_hi) << 32) | time_lo;
}

/*
 * L4 task new
 */
L4_INLINE l4_taskid_t
l4_task_new_sc(l4_taskid_t destination,
               l4_umword_t mcp_or_new_chief_and_flags,
               l4_umword_t esp,
               l4_umword_t eip,
               l4_threadid_t pager)
{
  unsigned dummy1, dummy2, dummy3, dummy4;
  l4_taskid_t new_task;

  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  %%edi, %%ebx	\n\t"	/* load dest id */
	  L4_SYSCALL(task_new)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl	 %%ebx		\n\t"
	 :
	  "=a" (dummy1),
	  "=c" (dummy2),
	  "=d" (dummy3),
	  "=S" (new_task.raw),
	  "=D" (dummy4)
	 :
	  "S" (destination.raw),
	  "D" (pager.raw),
	  "a" (mcp_or_new_chief_and_flags),
	  "c" (esp),
	  "d" (eip)
	);
  return new_task;
}

L4_INLINE int
l4_privctrl(l4_umword_t cmd,
	    l4_umword_t param)
{
  int err;
  unsigned dummy;

  __asm__ __volatile__(
	 "pushl %%ebx              \n\t"
	 "pushl %%ebp              \n\t"
	 L4_SYSCALL(privctrl)
         "popl  %%ebp              \n\t"
         "popl  %%ebx              \n\t"
	 :"=a"(err), "=d"(dummy)
	 :"0"(cmd),"1"(param));
  return err;
}

#endif

