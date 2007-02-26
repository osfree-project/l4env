/* 
 * $Id$
 */

#ifndef __L4_SYSCALLS_L4X0_GCC295_PIC_H__ 
#define __L4_SYSCALLS_L4X0_GCC295_PIC_H__ 


/*****************************************************************************
 *** L4 flex page unmap
 *****************************************************************************/
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
	  "0" (fpage),
	  "1" (map_mask)
	 :
	  "edx", "edi", "esi"
	 );
};

/*****************************************************************************
 *** L4 id myself
 *****************************************************************************/
L4_INLINE l4_threadid_t
l4_myself(void)
{
  l4_threadid_t temp_id;

  __asm__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  L4_SYSCALL(id_nearest)

	  FromId32_Esi                  /* my id ESI -> EDI/ESI */

	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  : 
	   "=S" (temp_id.lh.low),	/* ESI, 0 */
	   "=D" (temp_id.lh.high) 	/* EDI, 1 */
	  : 
	   "0" (0)			/* ESI, nil id (id.low = 0) */
	  :
	   "eax", "ecx", "edx"
	  );
  return temp_id;
}

/*****************************************************************************
 *** L4 id next chief
 *****************************************************************************/
L4_INLINE int 
l4_nchief(l4_threadid_t destination, 
	  l4_threadid_t * next_chief)
{
  int type;
  __asm__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */

	  ToId32_EdiEsi                 /* destination id EDI/ESI -> ESI */ 

	  L4_SYSCALL(id_nearest)

	  FromId32_Esi                  /* nearest id ESI -> EDI/ESI */

	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  : 
	   "=S" (next_chief->lh.low),	/* ESI, 0 */
	   "=D" (next_chief->lh.high),	/* EDI, 1 */
	   "=a" (type)			/* EAX, 2 */
	  : 
	   "0" (destination.lh.low),	/* ESI */
	   "1" (destination.lh.high) 	/* EDI */
	  :
	   "ecx", "edx"
	  );
  return type;
}

/*****************************************************************************
 *** L4 lthread_ex_regs
 *****************************************************************************/
L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination, 
		  l4_umword_t eip, 
		  l4_umword_t esp,
		  l4_threadid_t * preempter, 
		  l4_threadid_t * pager,
		  l4_umword_t * old_eflags, 
		  l4_umword_t * old_eip, 
		  l4_umword_t * old_esp)
{
  unsigned dummy1, dummy2;

  __asm__ __volatile__(
	  "pushl %%ebx  	\n\t"
	  "movl  %%edi, %%ebx	\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "pushl %%esi		\n\t"	/* save address of pager */
	  "pushl %%ebx		\n\t"	/* save address of preempter */
	  
	  "movl	4(%%esi), %%edi	\n\t"	/* load new pager id */	
	  "movl	 (%%esi), %%esi	\n\t"

	  "movl	4(%%ebx), %%ebp	\n\t"	/* load new preempter id */
	  "movl	 (%%ebx), %%ebx	\n\t"

	  ToId32_EdiEsi                 /* pager id     EDI/ESI -> ESI */
	  ToId32_EbpEbx                 /* preempter id EBP/EBX -> EBX */ 
	  
	  L4_SYSCALL(lthread_ex_regs)

	  FromId32_Esi                  /* old pager id     ESI -> EDI/ESI */
	  FromId32_Ebx                  /* old preempter id EBX -> EBP/EBX */

	  "xchgl (%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
					   and get address of preempter */
	  "movl	 %%ebp, 4(%%ebx)\n\t"	/* write preempter.lh.high */
	  "popl	 %%ebp		\n\t"	/* get preempter.lh.low */
	  "movl	 %%ebp, (%%ebx)	\n\t"	/* write preempter.lh.low */
	  "popl  %%ebx          \n\t"   /* get address of pager */
	  "movl  %%esi, (%%ebx) \n\t"   /* write pager */
	  "movl  %%edi, 4(%%ebx)\n\t"
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	  :
	  "=a" (*old_eflags),		/* EAX, 0 */
	  "=c" (*old_esp),		/* ECX, 1 */
	  "=d" (*old_eip),		/* EDX, 2 */
	  "=S" (dummy1),		/* ESI, clobbered output operand, 3 */
	  "=D" (dummy2) 		/* EDI, clobbered output operand, 4 */
	  :
	  "0" (destination.id.lthread),
	  "1" (esp),
	  "2" (eip),
	  "3" (pager),	 		/* ESI */
	  "4" (preempter)		/* EDI */
	  : "memory"
	  );
}

/*****************************************************************************
 *** L4 thread switch
 *****************************************************************************/
L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
  long dummy;
  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */

	  ToId32_EdiEsi                 /* destination id EDI/ESI -> ESI */

	  L4_SYSCALL(thread_switch)

	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	 : 
	  "=S" (dummy)
	 : 
	  "0" (destination.lh.low)
	 :  
	  "eax", "ecx", "edx", "edi"
	 );
}

/*****************************************************************************
 *** L4 thread schedule
 *****************************************************************************/
L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest, 
		   l4_sched_param_t param,
		   l4_threadid_t * ext_preempter, 
		   l4_threadid_t * partner,
		   l4_sched_param_t * old_param)
{
  l4_cpu_time_t temp;

  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "movl	 %%edx, %%ebx	\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "pushl %%ebx		\n\t"	/* save address of preempter */
	  "movl 4(%%ebx), %%ebp	\n\t"	/* load preempter id */
	  "movl  (%%ebx), %%ebx	\n\t"
	  "cmpl $-1,%%eax	\n\t"
	  "jz   1f		\n\t"	/* don't change if invalid */
	  "andl $0xfff0ffff, %%eax\n\t"	/* mask bits that must be zero */
	  "1:			\n\t"

	  ToId32_EdiEsi                 /* destination id EDI/ESI -> ESI */
	  ToId32_EbpEbx                 /* preempter id   EBP/EBX -> EBX */

	  L4_SYSCALL(thread_schedule)

	  FromId32_Esi                  /* partner id       ESI -> EDI/ESI */
	  FromId32_Ebx                  /* old preempter id EBX -> EBP/EBX */

	  "xchgl (%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
					   and get address of preempter */
	  "movl  %%ebp,4(%%ebx)	\n\t"	/* write preempter.lh.high */
	  "popl  %%ebp		\n\t"	/* get preempter.lh.high */
	  "movl  %%ebp, (%%ebx)	\n\t"	/* write preempter.lh.high */
	  "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl  %%ebx		\n\t"

	 : 
	  "=a" (*old_param), 		/* EAX, 0 */
	  "=c" (((l4_low_high_t*)&temp)->low), /* ECX, 1 */
	  "=d" (((l4_low_high_t*)&temp)->high),/* EDX, 2 */
	  "=S" (partner->lh.low),	/* ESI, 3 */
	  "=D" (partner->lh.high)	/* EDI, 4 */
	 : 
	  "2" (ext_preempter),   	/* EDX, 2 */
	  "0" (param),			/* EAX */
	  "3" (dest.lh.low),		/* ESI */
	  "4" (dest.lh.high)		/* EDI */
	 :
	  "memory"
	  );
  return temp;
}

/*****************************************************************************
 *** L4 task new
 *****************************************************************************/
L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination, 
	    l4_umword_t mcp_or_new_chief, 
	    l4_umword_t esp, 
	    l4_umword_t eip, 
	    l4_threadid_t pager)
{
  unsigned dummy1, dummy2, dummy3, dummy4, dummy0;
  __asm__ __volatile__(
	  "pushl %%ebx		\n\t"
	  "movl  %%edi, %%ebx	\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  4(%%ebx), %%ebp\n\t"	/* load pager id */
	  "movl   (%%ebx), %%ebx\n\t"
	  "pushl %%esi		\n\t"
	  "movl  4(%%esi), %%edi\n\t"
	  "movl   (%%esi), %%esi\n\t"

	  ToId32_EdiEsi                 /* destination id EDI/ESI -> ESI */
	  ToId32_EbpEbx                 /* pager id       EBP/EBX -> EBX */
	  ToId32_Eax                    /* new chief low      EAX -> EAX */

	  L4_SYSCALL(task_new)

	  FromId32_Esi                  /* new task ESI -> EDI/ESI */

	  "xchgl %%esi, (%%esp) \n\t"
	  "movl  %%edi, 4(%%esi)\n\t"
	  "popl  %%edi          \n\t"
	  "movl  %%edi, (%%esi) \n\t"
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	 : 
	  "=S" (dummy0),		/* ESI, 0 */
	  "=D" (dummy1),
	  "=a" (dummy2),		/* EAX, 2 */
	  "=c" (dummy3),		/* ECX, 3 */
	  "=d" (dummy4)			/* EDX, 4 */
	 :
	  "1" (&pager),			/* EDI, 1 */
	  "2" (mcp_or_new_chief),	/* EAX, 2 */
	  "3" (esp),			/* ECX, 3 */
	  "4" (eip),			/* EDX, 4 */
	  "0" (&destination)		/* ESI, 0 */
	 );
  return destination;
}

#endif

