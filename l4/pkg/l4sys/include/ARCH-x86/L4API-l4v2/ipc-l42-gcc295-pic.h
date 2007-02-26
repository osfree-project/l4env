/* 
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC295_PIC_H__ 
#define __L4_IPC_L42_GCC295_PIC_H__


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
    l4_umword_t d1, d2;
  } dwords = { snd_dword0, snd_dword1 };

  __asm__ __volatile__(
	  "pushl  %%ebx		\n\t"
	  "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "movl 4(%%edx),%%ebx	\n\t"
	  "movl  (%%edx),%%edx	\n\t"
	  "movl	  %%edi, %%ebp	\n\t"
	  "movl	4(%%esi),%%edi	\n\t"	/* dest.lh.high -> edi */
	  "movl	 (%%esi),%%esi	\n\t"	/* dest.lh.low  -> esi */
	  IPC_SYSENTER
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "movl  %%ebx,%%ecx	\n\t"
	  "popl  %%ebx		\n\t"
	 : 
	  "=a" (*result),		/* EAX, 0 */
	  "=d" (*rcv_dword0),		/* EDX, 1 */
	  "=c" (*rcv_dword1),		/* ECX, 2 */
	  "=S" (dummy1),		/* ESI, 3 */
	  "=D" (dummy2)			/* EDI, 4 */
	 :
	  "0" ((int)snd_msg),		/* EAX, 0 */
	  "1" (&dwords),		/* EDX, 1 */
	  "2" (timeout),		/* ECX, 2 */
	  "4" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)), /* EDI, 3 rcv msg -> EBP */
	  "3" (&dest)			/* ESI    */
	 :
	  "memory"
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
  unsigned dummy1, dummy2;

  struct {
    l4_threadid_t *dest;
    l4_threadid_t *src;
  } addresses = { &dest, src };
  struct {
    l4_umword_t d1, d2;
  } dwords = { snd_dword0, snd_dword1 };

  __asm__ __volatile__(
	/* eax, edx, ebx loaded, 
	 * edi contains rcv buffer address, must be moved to ebp,
	 * esi contains address of destination id,
	 * $5  address of src id */
	  "pushl  %%ebx		\n\t"
	  "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "movl  4(%%edx),%%ebx	\n\t"
	  "movl   (%%edx),%%edx	\n\t"
	  "pushl  %%esi		\n\t"
	  "movl   (%%esi), %%esi\n\t"	/* load address of dest */

	  "movl	  %%edi, %%ebp	\n\t" 	/* rmsg desc -> ebp */
	  "movl  4(%%esi),%%edi	\n\t"	/* dest.lh.high -> edi */
	  "movl	  (%%esi),%%esi	\n\t"	/* dest.lh.low  -> esi */
	  IPC_SYSENTER
	  "popl	  %%ebp		\n\t"
	  "movl	 4(%%ebp),%%ebp	\n\t"	/* load address of src */
	  "movl	  %%esi,(%%ebp)	\n\t"	/* esi -> src.lh.low  */
	  "movl	  %%edi,4(%%ebp)\n\t"	/* edi -> src.lh.high */

	  "popl	  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "movl   %%ebx,%%ecx	\n\t"
	  "popl   %%ebx		\n\t"
	 : 
	  "=a" (*result),		/* EAX, 0 */
	  "=d" (*rcv_dword0),		/* EDX, 1 */
	  "=c" (*rcv_dword1),		/* ECX, 2 */
	  "=S" (dummy1),		/* ESI, 3 */
	  "=D" (dummy2)			/* EDI, 4 */
	 :
	  "0" ((int)snd_msg),		/* EAX, 0 */
	  "1" (&dwords),		/* EDX, 1 */
	  "2" (timeout),		/* ECX, 2 */
	  "3" (&addresses),		/* ESI ,3 */
	  "4" (((int)rcv_msg) | L4_IPC_OPEN_IPC) /* EDI, 4  -> ebp rcv_msg */
	 :
	 "memory"
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
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  "movl  %%edi,%%ebx	\n\t"
	  "movl 4(%%esi),%%edi  \n\t"
	  "movl  (%%esi),%%esi  \n\t"
	  "movl	 $-1,%%ebp	\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
	  IPC_SYSENTER
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	  "popl  %%ebx		\n\t"
	 :
	  "=a" (*result),		/* EAX, 0 */
	  "=d" (dummy1),		/* EDX, 1 */
	  "=c" (dummy2),		/* ECX, 2 */
	  "=S" (dummy3),		/* ESI, 3 */
	  "=D" (dummy4)			/* EDI, 4 */
	 :
	  "0" ((int)snd_msg),		/* EAX, 0 */
	  "1" (snd_dword0),		/* EDX, 1 */
	  "2" (timeout),		/* ECX, 2 */
	  "3" (&dest),			/* ESI, 3 */
	  "4" (snd_dword1)		/* EDI, 4 */
	 :
	 "memory"
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
	  "pushl %%ebx		\n\t"
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "pushl %%esi		\n\t"	/* src */
	  "movl	 %%edx,%%ebp	\n\t"	/* rcv_msg */
	  IPC_SYSENTER
	  "popl  %%ebp		\n\t"	/* src */
	  "movl  %%esi,(%%ebp)	\n\t"	/* src.low */
	  "movl  %%edi,4(%%ebp)	\n\t"	/* src.hi */
	  "movl  %%ebx,%%ecx	\n\t"
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory
					   references ("m") before this point */
	  "popl  %%ebx		\n\t"
	 : 
	  "=a" (*result),		/* EAX, 0 */
	  "=c" (*rcv_dword1),		/* EBX, 1 */
	  "=d" (*rcv_dword0),		/* EDX, 2 */
	  "=S" (dummy1)			/* ESI, 3 */
	 :
	  "0" (L4_IPC_NIL_DESCRIPTOR),	/* EAX, 0 */
	  "1" (timeout),		/* ECX, 2 */
	  "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC), /* EDX, 1, rcv_msg -> EBP */
	  "3" (src)			/* ESI, 3 */
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
  unsigned dummy1;

  __asm__ __volatile__(
	  "pushl  %%ebx		\n\t"
	  "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "movl 4(%%esi),%%edi	\n\t"
	  "movl	 (%%esi),%%esi	\n\t"
	  "movl	  %%edx,%%ebp	\n\t" 
	  IPC_SYSENTER
	  "movl   %%ebx,%%ecx	\n\t"
	  "popl	  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	  "popl   %%ebx		\n\t"
	 : 
	  "=a" (*result),			/* EAX, 0 */
	  "=d" (*rcv_dword0),			/* EDX, 1 */
	  "=c" (*rcv_dword1),			/* ECX, 2 */
	  "=S" (dummy1)				/* ESI, 3 */
	 :
	  "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 */
	  "1" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),/* EDX, 2, rcv_msg -> EBP */
	  "2" (timeout),			/* ECX, 2 */
	  "3" (&src)				/* ESI, 3 */
	 :
	  "edi", "memory"
	 );
  return L4_IPC_ERROR(*result);
}

#endif

