/*
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC3_NOPIC_H__
#define __L4_IPC_L42_GCC3_NOPIC_H__


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
  unsigned dummy1, dummy2, dummy3;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %11, %%ebp		\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy1),
     "=S" (dummy2),
     "=D" (dummy3)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "c" (timeout),
     "S" (&dest),
     "ir"(((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
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
  unsigned dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %11, %%ebp		\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "c" (timeout),
     "S" (&dest),
     "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC)
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
  unsigned dummy1, dummy2, dummy3, dummy4, dummy5;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "orl   $-1,%%ebp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (dummy1),
     "=b" (dummy3),
     "=c" (dummy2),
     "=S" (dummy4),
     "=D" (dummy5)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "c" (timeout),
     "S" (dest.lh.low),
     "D" (dest.lh.high)
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
  unsigned dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
     "movl  %8,%%ebp	\n\t" /* rcv_msg */
     "xorl  %%edi,%%edi	\n\t" /* no absolute timeout !! */
     IPC_SYSENTER
     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_ipc_wait_next_period(l4_threadid_t *src,
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
     "movl  %8,%%ebp	\n\t" /* rcv_msg */
     IPC_SYSENTER
     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC),
     "S" (l4_next_period_id(L4_NIL_ID).lh.low),
     "D" (l4_next_period_id(L4_NIL_ID).lh.high) /* no absolute timeout */
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
  unsigned dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %8,%%ebp		\n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (src.lh.low),
     "D" (src.lh.high),
     "ir"(((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

