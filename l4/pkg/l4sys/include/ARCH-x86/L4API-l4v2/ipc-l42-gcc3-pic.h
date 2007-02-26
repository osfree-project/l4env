/* 
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC3_PIC_H__ 
#define __L4_IPC_L42_GCC3_PIC_H__


L4_INLINE int
l4_ipc_call(l4_threadid_t dest, 
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

  __asm__ __volatile__
    ("pushl  %%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %7,%%ebx	\n\t"
     "movl	  %10,%%ebp	\n\t"
     "movl   4(%%esi),%%edi\n\t"
     "movl    (%%esi),%%esi\n\t"
     IPC_SYSENTER
     "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx	\n\t"
     "popl  %%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1),
     "=S" (dummy1),
     "=D" (dummy2)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "g" (snd_dword1),
     "c" (timeout),
     "S" (&dest),
     "g" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_reply_and_wait(l4_threadid_t dest, 
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
  __asm__ __volatile__
    ("pushl  %%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %7,%%ebx	\n\t"
     "movl	  %10,%%ebp	\n\t" 	/* rmsg desc -> ebp */
     "movl   4(%%esi),%%edi\n\t"
     "movl    (%%esi),%%esi\n\t"
     IPC_SYSENTER

     "popl	  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl   %%ebx,%%ecx	\n\t"
     "popl   %%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "g" (snd_dword1),
     "c" (timeout),
     "S" (&dest),
     "g" (((int)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_ipc_send(l4_threadid_t dest, 
            const void *snd_msg, 
            l4_umword_t snd_dword0, 
            l4_umword_t snd_dword1, 
            l4_timeout_t timeout, 
            l4_msgdope_t *result)
{
  unsigned dummy1, dummy2, dummy3, dummy4;

  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %7,%%ebx	\n\t"
     "movl	 $-1,%%ebp	\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     "popl  %%ebx		\n\t"
     :
     "=a" (*result),
     "=d" (dummy1),
     "=c" (dummy2),
     "=S" (dummy3),
     "=D" (dummy4)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "g" (snd_dword1),
     "c" (timeout),
     "S" (dest.lh.low),
     "D" (dest.lh.high)
     );
  return L4_IPC_ERROR(*result);
};



L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg, 
            l4_umword_t *rcv_dword0, 
            l4_umword_t *rcv_dword1, 
            l4_timeout_t timeout, 
            l4_msgdope_t *result)
{
  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %7, %%ebp	\n\t"
     "xorl  %%edi, %%edi	\n\t"	/* no absolute timeout !! */
     IPC_SYSENTER
     "movl  %%ebx,%%ecx	\n\t"
     "popl	 %%ebp		\n\t"	/* restore ebp, no memory
					   references ("m") before this point */
     "popl  %%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "g" (((int)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg, 
               l4_umword_t *rcv_dword0, 
               l4_umword_t *rcv_dword1, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result)
{
  __asm__ __volatile__
    ("pushl  %%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl	  %7,%%ebp	\n\t" 
     IPC_SYSENTER
     "movl   %%ebx,%%ecx	\n\t"
     "popl	  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "popl   %%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (src.lh.low),
     "D" (src.lh.high),
     "g" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

