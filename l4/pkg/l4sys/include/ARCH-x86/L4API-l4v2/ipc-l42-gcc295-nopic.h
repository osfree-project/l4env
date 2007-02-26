/*
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC295_NOPIC_H__
#define __L4_IPC_L42_GCC295_NOPIC_H__


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
  unsigned dummy1;

  __asm__ __volatile__
    ("leal  %4, %%esi		\n\t"	/* address of dest id */
     "movl  %5, %%ecx		\n\t"	/* timeout */
     "pushl %%ebp		\n\t"	/* save ebp, no memory references
                                           ("m") after this point */
     "movl  %%edi, %%ebp	\n\t"
     "movl  4(%%esi), %%edi	\n\t"	/* dest.lh.high -> edi */
     "movl   (%%esi), %%esi	\n\t"	/* dest.lh.low  -> esi */
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
                                           ("m") before this point */
     :
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=D" (dummy1)
     :
     "m" (dest),
     "m" (timeout),
     "a" ((int)snd_msg),
     "b" (snd_dword1),
     "d" (snd_dword0),
     "D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "esi", "ecx", "memory"
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
  struct {
    l4_threadid_t *dest;
    l4_timeout_t timeout;
  } addresses = { &dest, timeout };

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
                                           ("m") after this point */
     "movl  4(%%esi), %%ecx	\n\t"	/* timeout -> ecx */
     "movl   (%%esi), %%esi	\n\t"	/* load address of dest */

     "movl  %%edi, %%ebp	\n\t" 	/* rmsg desc -> ebp */
     "movl  4(%%esi), %%edi	\n\t"	/* dest.lh.high -> edi */
     "movl   (%%esi), %%esi	\n\t"	/* dest.lh.low  -> esi */
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
                                           ("m") before this point */
     :
     "=a" (*result),
     "=b" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" ((int)snd_msg),
     "b" (snd_dword1),
     "d" (snd_dword0),
     "S" (&addresses),
     "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "ecx", "memory"
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
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
                                           ("m") after this point */
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"
     "orl   $-1,%%ebp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
                                           ("m") before this point */
     :
     "=a" (*result),
     "=c" (dummy2),
     "=b" (dummy3),
     "=d" (dummy1),
     "=S" (dummy4)
     :
     "a" ((int)snd_msg),
     "c" (timeout),
     "b" (snd_dword1),
     "d" (snd_dword0),
     "S" (&dest)
     :
     "edi", "memory"
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
  unsigned dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references
                                         ("m") after this point */
     "movl  %%ebx,%%ebp		\n\t" /* rcv_msg */
     "xorl  %%edi,%%edi		\n\t" /* no absolute timeout !! */
     IPC_SYSENTER
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
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "b" (((int)rcv_msg) | L4_IPC_OPEN_IPC),
     "c" (timeout)
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
  unsigned dummy1, dummy2, dummy3;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
                                           ("m") after this point */
     "orl   $-1, %%eax		\n\t"	/* nothing to send */
     "movl  %%ebx,%%ebp		\n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
                                           ("m") before this point */
     :
     "=b" (*rcv_dword1),
     "=c" (dummy1),
     "=d" (*rcv_dword0),
     "=S" (dummy2),
     "=D" (dummy3),
     "=a" (*result)
     :
     "b" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "c" (timeout),
     "S" (src.lh.low),
     "D" (src.lh.high)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

