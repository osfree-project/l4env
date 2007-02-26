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
};

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
	   "=S" (temp_id.lh.low),	/* ESI, 0 */
	   "=D" (temp_id.lh.high) 	/* EDI, 1 */
	  : 
	   "S" (0)			/* ESI, nil id (id.low = 0) */
	  :
	   "eax", "ecx", "edx"
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
	   "=S" (next_chief->lh.low),	/* ESI, 0 */
	   "=D" (next_chief->lh.high),	/* EDI, 1 */
	   "=a" (type)			/* EAX, 2 */
	  : 
	   "S" (destination.lh.low),	/* ESI */
	   "D" (destination.lh.high) 	/* EDI */
	  :
	   "ecx", "edx"
	  );
  return type;
}

/*
 * L4 lthread_ex_regs
 */
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
  __asm__ __volatile__(
	  "pushl %%ebx  	\n\t"
	  "movl  %%edi, %%ebx	\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "pushl %%ebx		\n\t"	/* save address of preempter */
	  
	  "movl	4(%%esi), %%edi	\n\t"	/* load new pager id */	
	  "movl	 (%%esi), %%esi	\n\t"

	  "movl	4(%%ebx), %%ebp	\n\t"	/* load new preempter id */
	  "movl	 (%%ebx), %%ebx	\n\t"

	  L4_SYSCALL(lthread_ex_regs)

	  "xchgl (%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
					   and get address of preempter */
	  "movl	 %%ebp, 4(%%ebx)\n\t"	/* write preempter.lh.high */
	  "popl	 (%%ebx)	\n\t"	/* write preempter.lh.low */
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  :
	  "=a" (*old_eflags),
	  "=c" (*old_esp),
	  "=d" (*old_eip),
	  "=S" (pager->lh.low),
	  "=D" (pager->lh.high)
	  :
	  "a" (destination.id.lthread),
	  "c" (esp),
	  "d" (eip),
	  "S" (pager),
	  "D" (preempter)
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
	  "=S" (dummy)
	 : 
	  "S" (destination.lh.low)
	 :
	  "eax", "ecx", "edx", "edi"
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
	  "popl  (%%ebx)	\n\t"	/* write preempter.lh.low */
	  "movl  %%ebp,4(%%ebx)	\n\t"	/* write preempter.lh.high */
	  "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"

	 : 
	  "=a" (*old_param),
	  "=c" (time_lo),
	  "=d" (time_hi),
	  "=S" (partner->lh.low),
	  "=D" (partner->lh.high)
	 : 
	  "a" (param),
	  "S" (dest.lh.low),
	  "D" (dest.lh.high),
	  "c" (ext_preempter)
	 :
	  "memory");
  return (((l4_cpu_time_t)time_hi) << 32) | time_lo;
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
  l4_taskid_t temp_id;
  unsigned dummy1, dummy2, dummy3;

  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "movl  4(%%edi), %%ebp\n\t"	/* load dest id */
	  "movl   (%%edi), %%ebx\n\t"
	  "movl  4(%%esi), %%edi\n\t"	/* load pager id */
	  "movl   (%%esi), %%esi\n\t"
	  L4_SYSCALL(task_new)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl	 %%ebx		\n\t"
	 : 
	  "=S" (temp_id.lh.low),
	  "=D" (temp_id.lh.high),
	  "=a" (dummy1),
	  "=c" (dummy2),
	  "=d" (dummy3)
	 :
	  "S" (&destination),
	  "D" (&pager),
	  "a" (mcp_or_new_chief),
	  "c" (esp),
	  "d" (eip)
	);
  return temp_id;
}

#endif

