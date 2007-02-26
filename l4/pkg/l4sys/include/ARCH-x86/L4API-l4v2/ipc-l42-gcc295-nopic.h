/*
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC295_NOPIC_H__
#define __L4_IPC_L42_GCC295_NOPIC_H__


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
  unsigned dummy1;

  __asm__ __volatile__(
	  "movl   %5, %%ecx	\n\t"	/* timeout */
	  "leal   %4, %%esi	\n\t"	/* address of dest id */
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  %%edi, %%ebp	\n\t"
	  "movl 4(%%esi), %%edi	\n\t"	/* dest.lh.high -> edi */
	  "movl	 (%%esi), %%esi	\n\t"	/* dest.lh.low  -> esi */
	  IPC_SYSENTER
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=a" (*result),		/* EAX, 0 */
	  "=d" (*rcv_dword0),		/* EDX, 1 */
	  "=b" (*rcv_dword1),		/* EBX, 2 */
	  "=D" (dummy1)			/* EDI, 3 */
	 :
	  "m" (dest),			/* dest, 4  */
	  "m" (timeout),		/* timeout, 5 */
	  "0" ((int)snd_msg), 		/* EAX, 0 */
	  "1" (snd_dword0),		/* EDX, 1 */
	  "2" (snd_dword1),		/* EBX, 2 */
	  "3" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* EDI, 3 rcv msg -> ebp */
	 :
	  "esi", "ecx", "memory"
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
  struct {
    l4_threadid_t *dest;
    l4_timeout_t timeout;
  } addresses = { &dest, timeout };

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  4(%%esi), %%ecx\n\t"	/* timeout -> ecx */
	  "movl   (%%esi), %%esi\n\t"	/* load address of dest */

	  "movl	 %%edi, %%ebp	\n\t" 	/* rmsg desc -> ebp */
	  "movl	4(%%esi), %%edi	\n\t"	/* dest.lh.high -> edi */
	  "movl	 (%%esi), %%esi	\n\t"	/* dest.lh.low  -> esi */
	  IPC_SYSENTER
	  "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=a" (*result),		/* EAX, 0 */
	  "=d" (*rcv_dword0),		/* EDX, 1 */
	  "=b" (*rcv_dword1),		/* EBX, 2 */
	  "=S" (src->lh.low),		/* ESI, 3 */
	  "=D" (src->lh.high)		/* EDI, 4 */
	 :
	  "0" ((int)snd_msg),		/* EAX, 0 */
	  "1" (snd_dword0),		/* EDX, 1 */
	  "2" (snd_dword1),		/* EBX, 2 */
	  "3" (&addresses),		/* ESI ,3 */
	  "4" (((int)rcv_msg) | L4_IPC_OPEN_IPC) /* edi, 4 -> ebp */
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
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl 4(%%esi),%%edi  \n\t"
	  "movl  (%%esi),%%esi  \n\t"
	  "movl  $-1,%%ebp	\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
	  IPC_SYSENTER
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
      :
      "=a" (*result),			/* EAX, 0 */
      "=d" (dummy1),			/* EDX, 1 */
      "=c" (dummy2),			/* ECX, 2 */
      "=b" (dummy3),			/* EBX, 3 */
      "=S" (dummy4)			/* ESI, 4 */
      :
      "0" ((int)snd_msg), 		/* EAX, 0 */
      "1" (snd_dword0),			/* EDX, 1 */
      "2" (timeout),			/* ECX, 2 */
      "3" (snd_dword1),			/* EBX, 3 */
      "4" (&dest)			/* ESI, 4 */
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
  unsigned dummy;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
	  "movl	 %%ebx,%%ebp	\n\t" /* rcv_msg */
	  IPC_SYSENTER
	  "popl	 %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
	 :
	  "=a" (*result),		/* EAX,0 */
	  "=b" (*rcv_dword1),		/* EBX,1 */
	  "=c" (dummy),			/* ECX, clobbered operand, 2 */
	  "=d" (*rcv_dword0),		/* EDX,3 */
	  "=S" (src->lh.low),		/* ESI,4 */
	  "=D" (src->lh.high)		/* EDI,5 */
	 :
	  "0" (L4_IPC_NIL_DESCRIPTOR),	/* EAX, 0 */
	  "1" (((int)rcv_msg) | L4_IPC_OPEN_IPC), /* EBX, 1, rcv_msg -> EBP */
	  "2" (timeout)			/* ECX, 2 */
	 :
	  "memory"
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
  unsigned dummy1, dummy2, dummy3;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  $-1, %%eax     \n\t"	/* nothing to send */
	  "movl	 %%ebx,%%ebp	\n\t"
	  IPC_SYSENTER
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=d" (*rcv_dword0),			/* EDX,0 */
	  "=b" (*rcv_dword1),			/* EBX,1 */
	  "=c" (dummy1),			/* ECX,2 */
	  "=S" (dummy2),			/* ESI,3 */
	  "=D" (dummy3),			/* EDI,4 */
	  "=a" (*result)			/* EAX,5 */
	 :
	  "1" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),/* EBX,1, rcv_msg -> EBP */
	  "2" (timeout),			/* ECX,2 */
	  "3" (src.lh.low),			/* ESI,3 */
	  "4" (src.lh.high)			/* EDI,4 */
	 :
	  "memory"
	 );
  return L4_IPC_ERROR(*result);
}

#endif

