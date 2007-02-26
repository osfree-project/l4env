
#include <l4/sys/ipc.h>

#ifdef L4_API_L4X0
#  error "L4 X.0 adaption not available!"
#endif

int
l4_ipc_call_static(l4_threadid_t dest, 
		   const void *snd_msg, l4_umword_t snd_dword0, l4_umword_t snd_dword1, 
		   void *rcv_msg, l4_umword_t *rcv_dword0, l4_umword_t *rcv_dword1, 
		   l4_timeout_t timeout, l4_msgdope_t *result)
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
	  "int	 $0x30		\n\t"
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
	  "3" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)), /* EDI, 3 rcv msg -> ebp */
	  "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),  /* EAX, 0  	*/
	  "1" (snd_dword0),		/* EDX, 1 */
	  "2" (snd_dword1)		/* EBX, 2 */
	 :
	  "esi", "ecx"
	  ,"memory"
      );
  return L4_IPC_ERROR(*result);
}

int
l4_ipc_reply_and_wait_static(l4_threadid_t dest, 
			   const void *snd_msg, 
			   l4_umword_t snd_dword0, l4_umword_t snd_dword1, 
			   l4_threadid_t *src,
			   void *rcv_msg, l4_umword_t *rcv_dword0, 
			   l4_umword_t *rcv_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result)
{
  unsigned dummy1, dummy2;

  struct {
    l4_threadid_t *dest;
    l4_threadid_t *src;
    l4_timeout_t timeout;
  } addresses = { &dest, src, timeout };

  __asm__ __volatile__(
	  /* eax, edx, ebx loaded, 
	   * edi contains rcv buffer address, must be moved to ebp,
	   * esi contains address of destination id,
	   * $5  address of src id */
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "pushl %%esi		\n\t"
	  "movl  8(%%esi), %%ecx\n\t"	/* timeout -> ecx */
	  "movl   (%%esi), %%esi\n\t"	/* load address of dest */

	  "movl	 %%edi, %%ebp	\n\t" 	/* rmsg desc -> ebp */
	  "movl	4(%%esi), %%edi	\n\t"	/* dest.lh.high -> edi */
	  "movl	 (%%esi), %%esi	\n\t"	/* dest.lh.low  -> esi */
	  "int	 $0x30		\n\t"
	  "popl	 %%ebp		\n\t"
	  "movl	4(%%ebp), %%ebp	\n\t"	/* load address of src */
	  "movl	 %%esi, (%%ebp)	\n\t"	/* esi -> src.lh.low  */
	  "movl	 %%edi, 4(%%ebp)\n\t"	/* edi -> src.lh.high */

	  "popl	%%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	 : 
	  "=a" (*result),		/* EAX, 0 */
	  "=d" (*rcv_dword0),		/* EDX, 1 */
	  "=b" (*rcv_dword1),		/* EBX, 2 */
	  "=S" (dummy1),
	  "=D" (dummy2)
	 :
	  "4" (((int)rcv_msg) | L4_IPC_OPEN_IPC), /* edi, 4  -> ebp rcv_msg */
	  "3" (&addresses),		/* ESI ,5 */
	  "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),/* EAX, 0 */
	  "1" (snd_dword0),		/* EDX, 1 */
	  "2" (snd_dword1)		/* EBX, 2 */
	 :
	  "ecx"
	 ,"memory"
      );
  return L4_IPC_ERROR(*result);
}

int
l4_ipc_send_static(l4_threadid_t dest, 
		   const void *snd_msg, l4_umword_t snd_dword0, l4_umword_t snd_dword1, 
		   l4_timeout_t timeout, l4_msgdope_t *result)
{
  unsigned dummy1, dummy2, dummy3, dummy4;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "movl 4(%%esi),%%edi  \n\t"
	  "movl  (%%esi),%%esi  \n\t"
	  "movl  $-1,%%ebp	\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
	  "int	 $0x30		\n\t"
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
      : 
      "=a" (*result),			/* EAX, 0 */
      "=d" (dummy1),			/* EDX, 1 */
      "=c" (dummy2),			/* ECX, 2 */
      "=b" (dummy3),			/* EBX, 3 */
      "=S" (dummy4)			/* ESI, 4 */
      :
      "1" (snd_dword0),			/* EDX, 1 */
      "2" (timeout),			/* ECX, 2 */
      "3" (snd_dword1),			/* EBX, 3 */
      "4" (&dest),			/* ESI, 4 */
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)) /* EAX, 0 */
      :
      "edi"
      , "memory"
      );
  return L4_IPC_ERROR(*result);
};

int
l4_ipc_wait_static(l4_threadid_t *src,
		   void *rcv_msg, l4_umword_t *rcv_dword0, l4_umword_t *rcv_dword1, 
		   l4_timeout_t timeout, l4_msgdope_t *result)
{
  unsigned dummy1, dummy2;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t" /* save ebp, no memory references 
					 ("m") after this point */
	  "pushl %%esi		\n\t" /* src */
	  "movl	 %%ebx,%%ebp	\n\t" /* rcv_msg */
	  "int	 $0x30		\n\t"
	  "popl  %%ebp		\n\t" /* src */
	  "movl  %%esi,(%%ebp)	\n\t" /* src.low */
	  "movl  %%edi,4(%%ebp)	\n\t" /* src.hi */
	  "popl	 %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
	 : 
	  "=a" (*result),		/* EAX,0 */
	  "=b" (*rcv_dword1),		/* EBX,1 */
	  "=c" (dummy1),		/* ECX, clobbered operand, 2 */
	  "=d" (*rcv_dword0),		/* EDX,3 */
	  "=S" (dummy2)			/* ESI, clobbered operand, 4 */
	 :
	  "0" (L4_IPC_NIL_DESCRIPTOR),	/* EAX, 0 */
	  "1" (((int)rcv_msg) | L4_IPC_OPEN_IPC), /* EBX, 1, rcv_msg -> EBP */
	  "2" (timeout),		/* ECX, 2 */
	  "4" (src)			/* ESI, 4 */
	 :
	  "edi"
	 ,"memory"
      );
  return L4_IPC_ERROR(*result);
}

extern int
l4_ipc_receive_static(l4_threadid_t src,
		      void *rcv_msg, l4_umword_t *rcv_dword0, l4_umword_t *rcv_dword1, 
		      l4_timeout_t timeout, l4_msgdope_t *result)
{
  unsigned dummy1, dummy2;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
	  "movl 4(%%esi), %%edi\n\t"
	  "movl	 (%%esi), %%esi \n\t"
	  "movl	 %%ebx,%%ebp	\n\t" 
	  "int	 $0x30		\n\t"
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
	 : 
	  "=a" (*result),			/* EAX,0 */
	  "=d" (*rcv_dword0),			/* EDX,1 */
	  "=b" (*rcv_dword1),			/* EBX,2 */
	  "=c" (dummy1),			/* ECX,3 */
	  "=S" (dummy2)				/* ESI,4 */
	 :
	  "3" (timeout),			/* ECX, 3 */
	  "4" (&src),				/* ESI, 4 */
	  "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	  "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC))/* EBX, 2, rcv_msg -> EBP */
	 :
	  "edi"
	 ,"memory"
	 );
  return L4_IPC_ERROR(*result);
}

