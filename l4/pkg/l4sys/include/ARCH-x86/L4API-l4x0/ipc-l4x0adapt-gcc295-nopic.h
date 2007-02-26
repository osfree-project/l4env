/* 
 * $Id$
 */

#ifndef __L4_IPC_L4X0_GCC295_NOPIC_H__ 
#define __L4_IPC_L4X0_GCC295_NOPIC_H__


/*****************************************************************************
 *** call
 *****************************************************************************/
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
  l4_umword_t dummy;

  __asm__ __volatile__
    ("movl  %5, %%ecx		\n\t"
     "leal  %4, %%esi		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %%edi, %%ebp	\n\t"
     "movl  4(%%esi), %%edi	\n\t"
     "movl   (%%esi), %%esi	\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongOut

     "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=D" (dummy)
     :
     "m" (dest),
     "m" (timeout),
     "a" ((l4_umword_t)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "D" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "esi", "ecx", "memory"
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
  l4_umword_t dummy;
  l4_umword_t dw[3] = { snd_dword0, snd_dword1, snd_dword2 };

  __asm__ __volatile__
    ("pushl  %%ecx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %%edi, %%ebp	\n\t"
     "movl   8(%%edx), %%edi	\n\t"
     "movl   4(%%edx), %%ebx	\n\t"
     "movl    (%%edx), %%edx	\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "popl  %%ecx		\n\t"
     : 
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy),
     "=D" (*rcv_dword2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dw[0]),
     "S" (l4sys_to_id32(dest)),
     "D" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

/*****************************************************************************
 *** reply an wait
 *****************************************************************************/
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
  struct
  {
    l4_threadid_t * dest;
    l4_timeout_t timeout;
  } addresses = { &dest, timeout };

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  4(%%esi), %%ecx	\n\t"
     "movl   (%%esi), %%esi	\n\t"

     "movl  %%edi, %%ebp	\n\t"
     "movl  4(%%esi), %%edi	\n\t"
     "movl   (%%esi), %%esi	\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongOut
     FromId32_Esi
                      
     "popl	%%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" ((l4_umword_t)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "S" (&addresses),
     "D" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "ecx", "memory"
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
  l4_umword_t dw[3] = { snd_dword0, snd_dword1, snd_dword2 };
  l4_umword_t src32;

  __asm__ __volatile__
    ("pushl %%ecx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %%edi, %%ebp	\n\t"
     "movl  8(%%edx), %%edi	\n\t"
     "movl  4(%%edx), %%ebx	\n\t"
     "movl   (%%edx), %%edx	\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "popl  %%ecx		\n\t"
     : 
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src32),
     "=D" (*rcv_dword2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dw[0]),
     "S" (l4sys_to_id32(dest)),
     "D" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "memory"
     );

  *src = l4sys_from_id32(src32);
  return L4_IPC_ERROR(*result);
}

/*****************************************************************************
 *** send
 *****************************************************************************/
L4_INLINE int
l4_ipc_send(l4_threadid_t dest, 
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1, 
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_umword_t dummy1, dummy2, dummy3, dummy4;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl 4(%%esi),%%edi	\n\t"
     "movl  (%%esi),%%esi	\n\t"
     "orl   $-1,%%ebp		\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongStackOut

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     : 
     "=a" (*result),
     "=d" (dummy1),
     "=c" (dummy2),
     "=b" (dummy3),
     "=S" (dummy4)
     :
     "a" ((l4_umword_t)snd_msg),
     "d" (snd_dword0),
     "c" (timeout),
     "b" (snd_dword1),
     "S" (&dest)
     :
     "edi", "memory"
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
  l4_umword_t dummy1, dummy2, dummy3;

  __asm__ __volatile__
    ("pushl %%edx		\n\t"
     "pushl %%edi		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "orl   $-1,%%ebp		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "popl  %%edi		\n\t"
     "popl  %%edx		\n\t"
     :
     "=a" (*result),
     "=b" (dummy1),
     "=c" (dummy2),
     "=S" (dummy3)
     :
     "a" ((l4_umword_t)snd_msg),
     "b" (snd_dword1),
     "c" (timeout),
     "d" (snd_dword0),
     "S" (l4sys_to_id32(dest)),
     "D" (snd_dword2)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

/*****************************************************************************
 *** wait
 *****************************************************************************/
L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg, 
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_umword_t dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references 
					 ("m") after this point */
     "movl  %%ebx,%%ebp		\n\t"

     "orl   $-1,%%eax		\n\t"
     FixLongStackIn

     IPC_SYSENTER

     FixLongOut
     FromId32_Esi

     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     : 
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=d" (*rcv_dword0),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "b" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "c" (timeout),
     "S" (0)			      /* no absolute timeout !! */
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
  l4_umword_t dummy;
  l4_umword_t src32;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references 
					 ("m") after this point */
     "movl  %%edx, %%ebp	\n\t"

     "xorl  %%esi, %%esi	\n\t" /* no absolute timeout! */
     "orl   $-1, %%eax		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     : 
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=d" (*rcv_dword0),
     "=S" (src32),
     "=D" (*rcv_dword2)
     :
     "c" (timeout),
     "d" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "memory"
     );
  *src = l4sys_from_id32(src32);
  return L4_IPC_ERROR(*result);
}

/*****************************************************************************
 *** receive
 *****************************************************************************/
L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1, 
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  l4_umword_t dummy1, dummy2;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  4(%%esi), %%edi	\n\t"
     "movl   (%%esi), %%esi	\n\t"
     "movl   %%ebx,%%ebp	\n\t" 

     ToId32_EdiEsi
     FixLongStackIn

     IPC_SYSENTER

     FixLongOut

     "popl	 %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     : 
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=c" (dummy1),
     "=d" (*rcv_dword0),
     "=S" (dummy2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "b" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "c" (timeout),
     "S" (&src)
     :
     "edi", "memory"
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
  l4_umword_t dummy1, dummy2;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %%edx,%%ebp		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     : 
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=c" (dummy1),
     "=d" (*rcv_dword0),
     "=S" (dummy2),
     "=D" (*rcv_dword2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "b" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "c" (timeout),
     "S" (l4sys_to_id32(src))
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif /* __L4_IPC_L4X0_GCC295_NOPIC_H__ */

