/* 
 * $Id$
 */

#ifndef __L4_IPC_L4X0_GCC3_PIC_H__ 
#define __L4_IPC_L4X0_GCC3_PIC_H__

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
  l4_umword_t dummy1, dummy2, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%11		\n\t"
     "movl  %9,%%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %10,%%ebp		\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongOut

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %11,%%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy1),
     "=D" (dummy2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (snd_dword0),
     "S" (&dest),
     "g" (snd_dword1),
     "ir"(((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "m" (ebx)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_call_w3(l4_threadid_t dest, 
               const void *snd_msg, 
               l4_umword_t snd_dword0, 
               l4_umword_t snd_dword1, 
               l4_umword_t snd_dword2, 
               void *rcv_msg, 
               l4_umword_t *rcv_dword0, 
               l4_umword_t *rcv_dword1, 
               l4_umword_t *rcv_dword2, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result)
{
  l4_umword_t dummy, ebx;
  l4_umword_t dw[3] = { snd_dword0, snd_dword1, snd_dword2 };

  __asm__ __volatile__
    ("movl  %%ebx,%10		\n\t"
     "movl  8(%%edx),%%edi	\n\t"
     "movl  4(%%edx),%%ebx	\n\t"
     "movl   (%%edx),%%edx	\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %9,%%ebp		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %10,%%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy),
     "=D" (*rcv_dword2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dw[0]),
     "S" (l4sys_to_id32(dest)),
     "ir"(((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
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
    ("movl  %%ebx,%11		\n\t"
     "movl  %9,%%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %10,%%ebp		\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongOut
     FromId32_Esi
                      
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %11,%%ebx		\n\t"
     :
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (snd_dword0),
     "S" (&dest),
     "g" (snd_dword1),
     "ir"(((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "m" (ebx)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_reply_and_wait_w3(l4_threadid_t dest, 
                         const void *snd_msg, 
                         l4_umword_t snd_dword0, 
                         l4_umword_t snd_dword1, 
                         l4_umword_t snd_dword2, 
                         l4_threadid_t *src,
                         void *rcv_msg, 
                         l4_umword_t *rcv_dword0, 
                         l4_umword_t *rcv_dword1, 
                         l4_umword_t *rcv_dword2, 
                         l4_timeout_t timeout, 
                         l4_msgdope_t *result)
{
  l4_umword_t src32, ebx;
  l4_umword_t dw[3] = { snd_dword0, snd_dword1, snd_dword2 };

  __asm__ __volatile__
    ("movl  %%ebx,%10		\n\t"
     "movl  8(%%edx),%%edi	\n\t"
     "movl  4(%%edx),%%ebx	\n\t"
     "movl   (%%edx),%%edx	\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %9,%%ebp		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %10,%%ebx		\n\t"
     :
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src32),
     "=D" (*rcv_dword2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dw[0]),
     "S" (l4sys_to_id32(dest)),
     "ir"(((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "m" (ebx)
     :
     "memory"
     );
  *src = l4sys_from_id32(src32);
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
  l4_umword_t dummy1, dummy2, dummy3, dummy4, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%11		\n\t"
     "movl  %10,%%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "orl   $-1,%%ebp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongStackOut

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %11,%%ebx		\n\t"
     :
     "=a" (*result),
     "=c" (dummy1),
     "=d" (dummy2),
     "=S" (dummy3),
     "=D" (dummy4)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (snd_dword0),
     "S" (dest.lh.low),
     "D" (dest.lh.high),
     "g" (snd_dword1),
     "m" (ebx)
     :
     "memory" /* necessary to ensure that writes to snd_msg aren't ignored */
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_send_w3(l4_threadid_t dest, 
               const void *snd_msg, 
               l4_umword_t snd_dword0, 
               l4_umword_t snd_dword1, 
               l4_umword_t snd_dword2, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result)
{
  l4_umword_t dummy1, dummy2, dummy3, dummy4, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%11		\n\t"
     "movl  %10,%%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "orl   $-1,%%ebp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %11,%%ebx		\n\t"
     :
     "=a" (*result),
     "=c" (dummy1),
     "=d" (dummy2),
     "=S" (dummy3),
     "=D" (dummy4)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (snd_dword0),
     "S" (l4sys_to_id32(dest)),
     "D" (snd_dword2),
     "g" (snd_dword1),
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
  unsigned ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%9		\n\t"
     "pushl %%ebp		\n\t" /* save ebp, no memory references 
					 ("m") after this point */
     "movl  %8,%%ebp		\n\t"

     FixLongStackIn

     IPC_SYSENTER

     FixLongOut
     FromId32_Esi

     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %9,%%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (0),			    /* no absolute timeout */
     "ir"(((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "m" (ebx)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_wait_w3(l4_threadid_t *src,
               void *rcv_msg, 
               l4_umword_t *rcv_dword0, 
               l4_umword_t *rcv_dword1, 
               l4_umword_t *rcv_dword2, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result)
{
  l4_umword_t src32, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%9		\n\t"
     "pushl %%ebp		\n\t" /* save ebp, no memory references 
					 ("m") after this point */
     "movl  %7,%%ebp		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %9,%%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src32),
     "=D" (*rcv_dword2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (0),			    /* no absolute timeout */
     "ir"(((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "m" (ebx)
     :
     "memory"
     );
  *src = l4sys_from_id32(src32);
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
  l4_umword_t dummy1, dummy2, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%10		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %9,%%ebp		\n\t" 

     ToId32_EdiEsi
     FixLongStackIn

     IPC_SYSENTER

     FixLongOut

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %10,%%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy1),
     "=D" (dummy2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (src.lh.low),
     "D" (src.lh.high),
     "ir"(((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "m" (ebx)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_receive_w3(l4_threadid_t src,
                  void *rcv_msg, 
                  l4_umword_t *rcv_dword0, 
                  l4_umword_t *rcv_dword1, 
                  l4_umword_t *rcv_dword2, 
                  l4_timeout_t timeout, 
                  l4_msgdope_t *result)
{
  l4_umword_t dummy, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%9		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %8,%%ebp		\n\t" 

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "movl  %9,%%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy),
     "=D" (*rcv_dword2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (l4sys_to_id32(src)),
     "g" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "m" (ebx)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

