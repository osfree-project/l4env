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
  unsigned dummy1, dummy2, ebx;

  __asm__ __volatile__
    ("movl   %%ebx,%11		\n\t"
     "movl   %7,%%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %10,%%ebp		\n\t"
     "movl   4(%%esi),%%edi	\n\t"
     "movl    (%%esi),%%esi	\n\t"
     IPC_SYSENTER
     "popl   %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl   %%ebx,%%ecx	\n\t"
     "movl   %11,%%ebx		\n\t"
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
     "ir"(((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "m" (ebx)
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
  l4_umword_t ebx;

  __asm__ __volatile__
    ("movl   %%ebx,%11		\n\t"
     "movl   %7,%%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %10,%%ebp		\n\t" 	/* rmsg desc -> ebp */
     "movl   4(%%esi),%%edi	\n\t"
     "movl    (%%esi),%%esi	\n\t"
     IPC_SYSENTER

     "popl   %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl   %%ebx,%%ecx	\n\t"
     "movl   %11,%%ebx		\n\t"
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
     "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC),
     "m" (ebx)
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
  unsigned dummy1, dummy2, dummy3, dummy4, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%11		\n\t"
     "movl  %7,%%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  $-1,%%ebp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     "movl  %11,%%ebx		\n\t"
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
     "D" (dest.lh.high),
     "m" (ebx)
     :
     "memory" /* necessary to ensure that writes to snd_msg aren't ignored */
     );
  return L4_IPC_ERROR(*result);
}



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
     "movl  %%edx,%%ebp		\n\t"
     "xorl  %%edi,%%edi		\n\t"	/* no absolute timeout !! */
     IPC_SYSENTER
     "movl  %%ebx,%%ecx		\n\t"
     "popl  %%ebp		\n\t"	/* restore ebp, no memory
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
     "d" (((int)rcv_msg) | L4_IPC_OPEN_IPC)
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
     "movl   %%edx,%%ebp	\n\t"
     IPC_SYSENTER
     "movl   %%ebx,%%ecx	\n\t"
     "popl   %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "popl   %%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "d" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "S" (src.lh.low),
     "D" (src.lh.high)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

