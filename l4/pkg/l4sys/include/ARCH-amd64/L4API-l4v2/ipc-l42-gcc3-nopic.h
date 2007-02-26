#ifndef __L4_IPC_L42_GCC3_NOPIC_H__
#define __L4_IPC_L42_GCC3_NOPIC_H__

// XXX hacked
typedef unsigned long long int Unsigned64;
typedef signed long long int Signed64;

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
  unsigned long dummy1, dummy2, dummy3;

  __asm__ __volatile__
    ("push %%rbp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "mov  %11, %%rbp		\n\t"
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy1),
     "=S" (dummy2),
     "=D" (dummy3)
     :
     "a" ((Signed64)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "D" (timeout),
     "S" (dest),		
     "ir"(((Signed64)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory"
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
  unsigned long dummy1, dummy2;

  __asm__ __volatile__
    ("push %%rbp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "mov  %11, %%rbp		\n\t"
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy1),
     "=D" (dummy2),
     "=S" (*src)
     :
     "a" ((Signed64)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "D" (timeout),
     "S" (dest),	
     "ir"(((Signed64)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory"
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
  unsigned long dummy1, dummy2, dummy3, dummy4, dummy5;

  __asm__ __volatile__
    ("push %%rbp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "mov  $-1,%%rbp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (dummy1),
     "=b" (dummy3),
     "=c" (dummy2),
     "=S" (dummy4),
     "=D" (dummy5)
     :
     "a" ((Signed64)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "D" (timeout),
     "S" (dest)
     :
     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
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
  unsigned long dummy, dummy1;

  __asm__ __volatile__
    ("push %%rbp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
     "mov  %8,%%rbp	\n\t" /* rcv_msg */
     "xor  %%rsi,%%rsi	\n\t" /* no absolute timeout !! */
     IPC_SYSENTER
     "pop  %%rbp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (*src),
     "=D" (dummy1)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "D" (timeout),
     "ir"(((Signed64)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory"
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
  unsigned long dummy, dummy1;

  __asm__ __volatile__
    ("push %%rbp		\n\t"	/* save rbp, no memory references
					   ("m") after this point */
     "mov  %8,%%rbp		\n\t"
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore rbp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=D" (dummy1)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "D" (timeout),
     "S" (src),	
     "ir"(((Signed64)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

