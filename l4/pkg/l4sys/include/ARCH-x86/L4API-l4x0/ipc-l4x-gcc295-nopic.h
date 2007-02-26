#ifndef __L4_IPC_L4X_GCC295_NOPIC_H__
#define __L4_IPC_L4X_GCC295_NOPIC_H__

#error This file is currently not used by anyone, is it??

/*
 * Implementation
 */

#ifdef CONFIG_L4_CALL_SYSCALLS 
#  ifdef CONFIG_L4_ABS_SYSCALLS
#    define IPC_SYSENTER "call __L4_ipc_direct  \n\t"
#  else
#    define IPC_SYSENTER "call *__L4_ipc        \n\t"
#  endif
#else
#  ifndef CONFIG_L4_IPC_SYSENTER
#    define IPC_SYSENTER "int    $0x30          \n\t"
#  else
#    define IPC_SYSENTER \
        "push   %%ecx          \n\t"                              \
        "push   %%ebp          \n\t"  /* ignored by Fiasco    */  \
        "push   $0x1b          \n\t"  /* ignored by Fiasco    */  \
        "push   $0f            \n\t"  /* offset ret_addr      */  \
        "mov    %%esp,%%ecx    \n\t"                              \
        "sysenter              \n\t"  /* = db 0x0F,0x34       */  \
        "mov    %%ebp,%%edx    \n\t"                              \
        "0:                    \n\t"
#  endif
#endif


L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, 
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  unsigned dummy1, dummy2;
  struct {
    l4_threadid_t dest;
    l4_timeout_t timeout;
  } parameters = { dest, timeout };
  __asm__ __volatile__(
      "movl	4(%%esi), %%ecx	\n\t"		/* timeout */
      "movl	(%%esi), %%esi	\n\t"		/* dest id */
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%edi, %%ebp	\n\t"
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=S" (dummy1),				/* ESI,3 */
      "=D" (dummy2)				/* EDI,4 */
      :
      "3" (&parameters),			/* dest,5 */
      "4" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),/* EDI,6, rcv msg -> ebp */
      "0" ((int)snd_msg),			/* EAX,7 */
      "1" (snd_dword0),				/* EDX,8 */
      "2" (snd_dword1)				/* EBX,9 */
      :
       "ecx", "memory"
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   l4_umword_t snd_dword0, 
			   l4_umword_t snd_dword1, 
			   l4_threadid_t *src, 
			   void *rcv_msg, 
			   l4_umword_t *rcv_dword0, 
			   l4_umword_t *rcv_dword1, 
			   l4_timeout_t timeout, 
			   l4_msgdope_t *result)
{
  unsigned dummy1;
  struct {
    l4_threadid_t dest;
    l4_timeout_t timeout;
  } parameters = { dest, timeout };
  
  __asm__ __volatile__(
      "movl     4(%%esi), %%ecx \n\t"           /* timeout -> ecx */
      "movl	(%%esi), %%esi	\n\t"		/* load address of dest */

      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */

      "movl	%%edi, %%ebp    \n\t" 		/* rmsg desc -> ebp */
      IPC_SYSENTER
      
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=S" (*src),				/* ESI,3 */
      "=D" (dummy1)				/* EDI,4 */
      :
      "3" (&parameters),			/* ESI,5 */
      "4" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* EDI,6  -> ebp rcv_msg */
      "0" ((int)snd_msg),			/* EAX,7 */
      "1" (snd_dword0),			        /* EDX,8 */
      "2" (snd_dword1)				/* EBX,9 */
      :
      "ecx", "memory"
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, 
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  unsigned dummy1, dummy2, dummy3, dummy4;
  __asm__ __volatile__(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */

      "movl	$-1 ,%%ebp	\n\t" 		/* L4_IPC_NIL_DESCRIPTOR */
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=b" (dummy1),				/* EBX,1 */
      "=c" (dummy2),				/* ECX,2 */
      "=d" (dummy3),				/* EDX,3 */
      "=S" (dummy4)				/* ESI,4 */
      :
      "3" (snd_dword0),				/* EDX,5 */
      "2" (timeout),				/* ECX,6 */
      "1" (snd_dword1),				/* EBX,7 */
      "4" (dest.dw),			        /* ESI,8 */
      "0" ((int)snd_msg)			/* EAX,9 */
      :
        "edi", "memory"
      );
  return L4_IPC_ERROR(*result);
};


L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1,
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  unsigned dummy1;
  __asm__ __volatile__(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%ebx,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=c" (dummy1),				/* ECX,3 */
      "=S" (src->dw)			        /* ESI,4 */
      :
      "3" (timeout),				/* ECX,5 */
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX,6 */
      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* EBX,7, rcv_msg -> EBP */
      :
       "edi", "memory"
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, 
		    l4_umword_t *rcv_dword0, 
		    l4_umword_t *rcv_dword1, 
		    l4_timeout_t timeout, 
		    l4_msgdope_t *result)
{
  unsigned dummy1, dummy2;
  __asm__ __volatile__(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%ebx,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      :
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=c" (dummy1),				/* ECX,3 */
      "=S" (dummy2)				/* ESI,4 */
      :
      "3" (timeout),				/* ECX,5 */
      "4" (src.dw),				/* ESI,6 */
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX,7 */
      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* EBX,8, rcv_msg -> EBP */
      :
      "edi", "memory"
      );
  return L4_IPC_ERROR(*result);
}

#endif /* __L4_IPC_L4X_GCC295_NOPIC_H__ */
