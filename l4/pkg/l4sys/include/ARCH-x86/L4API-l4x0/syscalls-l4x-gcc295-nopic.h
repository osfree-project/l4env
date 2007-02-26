#ifndef __L4_SYSCALLS_L4X_GCC295_NOPIC_H__
#define __L4_SYSCALLS_L4X_GCC295_NOPIC_H__

#define L4_FP_REMAP_PAGE	0x00	/* Page is set to read only */
#define L4_FP_FLUSH_PAGE	0x02	/* Page is flushed completly */
#define L4_FP_OTHER_SPACES	0x00	/* Page is flushed in all other */
					/* address spaces */
#define L4_FP_ALL_SPACES	0x80000000U
					/* Page is flushed in own address */ 
					/* space too */

#define L4_NC_SAME_CLAN		0x00	/* destination resides within the */
					/* same clan */
#define L4_NC_INNER_CLAN	0x0C	/* destination is in an inner clan */
#define L4_NC_OUTER_CLAN	0x04	/* destination is outside the */
					/* invoker's clan */

#define L4_CT_LIMITED_IO	0
#define L4_CT_UNLIMITED_IO	1
#define L4_CT_DI_FORBIDDEN	0
#define L4_CT_DI_ALLOWED	1

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
		  l4_umword_t eip,
		  l4_umword_t esp,
		  l4_threadid_t *preempter,
		  l4_threadid_t *pager,
		  l4_umword_t *old_eflags,
		  l4_umword_t *old_eip,
		  l4_umword_t *old_esp);
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
	    l4_umword_t mcp_or_new_chief, 
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager);


/*
 * Implementation
 */

#define SYSCALL_ipc              "int $0x30 \n\t"
#define SYSCALL_id_nearest       "int $0x31 \n\t"
#define SYSCALL_fpage_unmap      "int $0x32 \n\t"
#define SYSCALL_thread_switch    "int $0x33 \n\t" 
#define SYSCALL_thread_schedule  "int $0x34 \n\t"
#define SYSCALL_lthread_ex_regs  "int $0x35 \n\t"
#define SYSCALL_task_new         "int $0x36 \n\t"

#ifdef CONFIG_L4_CALL_SYSCALLS
#  ifdef CONFIG_L4_ABS_SYSCALLS
#    define L4_SYSCALL(s) "call __L4_"#s"_direct  \n\t"
#  else
#    define L4_SYSCALL(s) "call *__L4_"#s"  \n\t"
#  endif
#else
#  define L4_SYSCALL(s) SYSCALL_##s
#endif

 
L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask)
{
  unsigned dummy1, dummy2;
  __asm__ __volatile__(
		 "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 L4_SYSCALL(fpage_unmap)
		 "popl	%%ebp	\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */

		 : 
		 "=a" (dummy1),
		 "=c" (dummy2)
		 /* No output */
		 : 
		 "0" (fpage),
		 "1" (map_mask)
		 :
		 "ebx", "edx", "edi", "esi"
		 );
}

L4_INLINE l4_threadid_t
l4_myself(void)
{
  l4_threadid_t temp_id;

  __asm__ __volatile__(
	  "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
	  L4_SYSCALL(id_nearest)
	  "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
	  : 
	  "=S" (temp_id.dw)	        /* ESI, 0 */
	  : 
	  "0" (0)			/* ESI, nil id (id.low = 0) */
	  :
	  "eax", "ebx", "ecx", "edx", "edi"
	  );
  return temp_id;
}


L4_INLINE int 
l4_nchief(l4_threadid_t destination,
	  l4_threadid_t *next_chief)
{
  int type;
  __asm__ __volatile__(
	  "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
	  L4_SYSCALL(id_nearest)
	  "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
	  : 
	  "=S" (next_chief->dw),	/* ESI,0 */
	  "=a" (type)			/* EAX,1 */
	  : 
	  "0" (destination.dw)	        /* ESI,2 */
	  :
	  "ebx", "ecx", "edx", "edi"
	  );
  return type;
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
  __asm__ __volatile__(
	  "pushl	%%ebp	\n\t"	/* save ebp, no memory 
					   references ("m") after 
					   this point */
	  L4_SYSCALL(lthread_ex_regs)
	  "popl	%%ebp		\n\t"	/* restore ebp, no memory 
					   references ("m") before 
					   this point */
	  :
	  "=a" (*old_eflags),		/* EAX,0 */
	  "=c" (*old_esp),		/* ECX,1 */
	  "=d" (*old_eip),		/* EDX,2 */
	  "=S" (pager->dw),		/* ESI,3 */
	  "=b" (preempter->dw)          /* EBX,4 */
	  :
	  "0" (destination.id.lthread),	/* EAX,5 */
	  "1" (esp),			/* ECX,6 */
	  "2" (eip),			/* EDX,7 */
	  "3" (pager->dw), 		/* ESI,8 */
	  "4" (preempter->dw)		/* EBX,9 */
	  : 
	  "edi"
	  );
}


L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
  unsigned dummy;
  __asm__ __volatile__(
		 "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 L4_SYSCALL(thread_switch)
		 "popl	%%ebp	\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
		 : 
		 "=S" (dummy),
		 "=a" (dummy)
		 : 
		 "0" (destination.dw),
		 "1" (0)			/* Fiasco requirement */
		 :
		 "ebx", "ecx", "edx", "edi"
		 );
}


L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param)
{
  l4_cpu_time_t temp;

  __asm__ __volatile__(
		 "pushl %%ebp		\n\t"	/* save ebp, no memory 
						   references ("m") after 
						   this point */
/* 		 asm_enter_kdebug("before calling thread_schedule") */
		 L4_SYSCALL(thread_schedule)
/* 		 asm_enter_kdebug("after calling thread_schedule") */

		 "popl	%%ebp		\n\t"	/* restore ebp, no memory 
						   references ("m") before 
						   this point */
		 : 
		 "=a" (*old_param), 			/* EAX,0 */
		 "=c" (((l4_low_high_t *)&temp)->low),	/* ECX,1 */
		 "=d" (((l4_low_high_t *)&temp)->high),	/* EDX,2 */
		 "=S" (partner->dw),		        /* ESI,3 */
		 "=b" (ext_preempter->dw)		/* EBX,4 */
		 : 
		 "4" (ext_preempter->dw),		/* EBX,5 */
		 "0" (param),				/* EAX,6 */
		 "3" (dest.dw)				/* ESI,7 */
		 :
		 "edi"
		 );
  return temp;
}


L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief, 
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager)
{
  unsigned dummy1, dummy2, dummy3, dummy4;
  l4_taskid_t temp_id;
  __asm__ __volatile__(
		 "pushl	%%ebp		\n\t"	/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 L4_SYSCALL(task_new)
		 "popl	%%ebp		\n\t"	/* restore ebp, no memory 
						   references ("m") before 
						   this point */
		 : 
		 "=a" (dummy1),			/* EAX,0 */
		 "=b" (dummy2),			/* EBX,1 */
		 "=c" (dummy3),			/* ECX,2 */
		 "=d" (dummy4),			/* EDX,3 */
		 "=S" (temp_id.dw)		/* ESI,4 */
		 :
		 "1" (pager.dw),		/* EBX,5 */
		 "0" (mcp_or_new_chief),	/* EAX,6 */
		 "2" (esp),			/* ECX,7 */
		 "3" (eip),			/* EDX,8 */
		 "4" (destination.dw)	        /* ESI,9 */
		 :
		  "edi"
		 );
  return temp_id;
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

#endif /* __L4_SYSCALLS_L4X_GCC295_NOPIC_H__ */
