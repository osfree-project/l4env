#ifndef __L4_IPC_L42_GCC3_NOPIC_H__
#define __L4_IPC_L42_GCC3_NOPIC_H__

#ifdef __PIC__
# define PIC_CLOBBER
# define PIC_PRESERVE " push %%rbx \n\t"
# define PIC_RESTORE  " pop  %%rbx \n\t"
#else
# define PIC_CLOBBER , "rbx"
# define PIC_PRESERVE
# define PIC_RESTORE
#endif

// XXX hacked
typedef unsigned long long int Unsigned64;
typedef signed long long int Signed64;

L4_INLINE int
l4_ipc_call_tag(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
	    l4_msgtag_t tag,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
	    l4_msgtag_t *rtag)
{
  unsigned long dummy2, dummy3;

  register unsigned long r8 asm("r8") = snd_dword1;

  __asm__ __volatile__ (
      PIC_PRESERVE
      "push %%rbp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
      "mov  %[rcv_desc], %%rbp		\n\t"
      IPC_SYSENTER
      "pop  %%rbp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
      PIC_RESTORE
      :
      "=a" (*result),
      "=d" (*rcv_dword0),
      "=r" (r8),
      "=S" (dummy2),
      "=D" (dummy3),
      "=c" (rtag->raw)
      :
      "a" ((Signed64)snd_msg),
      "d" (snd_dword0),
      "r" (r8),
      "D" (timeout),
      "S" (dest),
      "c" (tag.raw),
      [rcv_desc] "ir"(((Signed64)rcv_msg) & (~L4_IPC_OPEN_IPC))
      :
      "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory" PIC_CLOBBER
  );
  *rcv_dword1 = r8;
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_ipc_reply_and_wait_tag(l4_threadid_t dest,
                      const void *snd_msg,
                      l4_umword_t snd_dword0,
                      l4_umword_t snd_dword1,
                      l4_msgtag_t tag,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_dword0,
                      l4_umword_t *rcv_dword1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result,
                      l4_msgtag_t *rtag)
{
  unsigned long dummy2;
  register unsigned long r8 asm("r8") = snd_dword1;

  __asm__ __volatile__ (
     PIC_PRESERVE
     "push %%rbp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "mov  %[rcv_desc], %%rbp		\n\t"
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     PIC_RESTORE
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=r" (r8),
     "=D" (dummy2),
     "=S" (*src),
     "=c" (rtag->raw)
     :
     "a" ((Signed64)snd_msg),
     "d" (snd_dword0),
     "r" (r8),
     "D" (timeout),
     "S" (dest),
     "c" (tag.raw),
     [rcv_desc] "ir"(((Signed64)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory" PIC_CLOBBER
     );
  *rcv_dword1 = r8;
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_ipc_send_tag(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
	    l4_msgtag_t tag,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  unsigned long dummy1, dummy2, dummy4, dummy5;
  register unsigned long r8 asm("r8") = snd_dword1;

  __asm__ __volatile__ (
     PIC_PRESERVE
     "push %%rbp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "mov  $-1,%%rbp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     PIC_RESTORE
     :
     "=a" (*result),
     "=d" (dummy1),
     "=r" (r8),
     "=c" (dummy2),
     "=S" (dummy4),
     "=D" (dummy5)
     :
     "a" ((Signed64)snd_msg),
     "d" (snd_dword0),
     "r" (r8),
     "D" (timeout),
     "S" (dest),
     "c" (tag.raw)
     :
     "r9", "r10", "r11", "r12", "r13", "r14", "r15" PIC_CLOBBER
     );
  return L4_IPC_ERROR(*result);
};


L4_INLINE int
l4_ipc_wait_tag(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
	    l4_msgtag_t *rtag)
{
  unsigned long dummy1;
  register unsigned long r8 asm("r8");

  __asm__ __volatile__
    (PIC_PRESERVE
     "push %%rbp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
     "mov  %[rcv_desc],%%rbp	\n\t" /* rcv_msg */
     IPC_SYSENTER
     "pop  %%rbp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     PIC_RESTORE
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=r" (r8),
     "=S" (*src),
     "=D" (dummy1),
     "=c" (rtag->raw)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "D" (timeout),
     [rcv_desc] "ir"(((Signed64)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory" PIC_CLOBBER
     );
  *rcv_dword1 = r8;
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_ipc_receive_tag(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1,
               l4_timeout_t timeout,
               l4_msgdope_t *result,
	       l4_msgtag_t *rtag)
{
  unsigned long dummy1;
  register unsigned long r8 asm("r8");

  __asm__ __volatile__ (
     PIC_PRESERVE
     "push %%rbp		\n\t"	/* save rbp, no memory references
					   ("m") after this point */
     "mov  %[rcv_desc],%%rbp		\n\t"
     IPC_SYSENTER
     "pop  %%rbp		\n\t"	/* restore rbp, no memory references
					   ("m") before this point */
     PIC_RESTORE
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=r" (r8),
     "=D" (dummy1),
     "=c" (rtag->raw)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "D" (timeout),
     "S" (src),
     [rcv_desc] "ir"(((Signed64)rcv_msg) & (~L4_IPC_OPEN_IPC))
     :
     "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory" PIC_CLOBBER
     );
  *rcv_dword1 = r8;
  return L4_IPC_ERROR(*result);
}

#undef PIC_CLOBBER
#undef PIC_PRESERVE
#undef PIC_RESTORE

#endif

