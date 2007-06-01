/* 
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC3_PIC_H__ 
#define __L4_IPC_L42_GCC3_PIC_H__


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
  unsigned dummy1, ebx;
  struct { unsigned long a,b,c; } msg = {snd_dword0, snd_dword1, tag.raw};

  __asm__ __volatile__
    ("pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %%ebx,%[ebx]	\n\t"
     "movl   %[rcv_msg],%%ebp	\n\t"
     "movl   4(%[msg]),%%ebx	\n\t"
     "movl   8(%[msg]),%%edi	\n\t"
     "movl   (%[msg]), %%edx	\n\t"
     IPC_SYSENTER
     "popl   %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl   %%ebx,%%ecx	\n\t"
     "movl   %[ebx],%%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1),
     "=S" (dummy1),
     "=D" (rtag->raw)
     :
     "a" ((int)snd_msg),
     [msg] "d" (&msg),
     "c" (timeout),
     "S" (dest.raw),
     [rcv_msg] "ir"(((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     [ebx] "m" (ebx),
     "m" (msg)
	     
     :
     "memory"
     );
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
  l4_umword_t ebx;
  struct { unsigned long a,b,c; } msg = {snd_dword0,snd_dword1,tag.raw};

  __asm__ __volatile__
    ("pushl  %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl   %%ebx,%[ebx]	\n\t"
     "movl  %[msg_desc],%%ebp	\n\t"	/* rmsg desc -> ebp */
     "movl  4(%[msg]), %%ebx	\n\t"
     "movl  8(%[msg]), %%edi	\n\t"
     "movl  (%[msg]), %%edx	\n\t"
     IPC_SYSENTER

     "popl   %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     "movl   %%ebx,%%ecx	\n\t"
     "movl   %[ebx],%%ebx	\n\t"
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1),
     "=S" (src->raw),
     "=D" (rtag->raw)
     :
     "a" ((int)snd_msg),
     [msg] "d" (&msg),
     "c" (timeout),
     "S" (dest.raw),
     [msg_desc] "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC),
     [ebx] "m" (ebx),
     "m" (msg)
     :
     "memory");
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
  unsigned dummy1, dummy2, dummy3, dummy4, ebx;

  __asm__ __volatile__
    ("movl  %%ebx,%[ebx]	\n\t"
     "movl  %7,%%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  $-1,%%ebp		\n\t"	/* L4_IPC_NIL_DESCRIPTOR */
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     "movl  %[ebx],%%ebx	\n\t"
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
     "S" (dest.raw),
     "D" (tag.raw),
     [ebx] "m" (ebx)
     :
     "memory" /* necessary to ensure that writes to snd_msg aren't ignored */
     );
  return L4_IPC_ERROR(*result);
}



L4_INLINE int
l4_ipc_wait_tag(l4_threadid_t *src,
            void *rcv_msg, 
            l4_umword_t *rcv_dword0, 
            l4_umword_t *rcv_dword1, 
            l4_timeout_t timeout, 
            l4_msgdope_t *result,
	    l4_msgtag_t *tag)
{
  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %%edx,%%ebp		\n\t"
     IPC_SYSENTER
     "movl  %%ebx,%%ecx		\n\t"
     "popl  %%ebp		\n\t"	/* restore ebp, no memory
					   references ("m") before this point */
     "popl  %%ebx		\n\t"
     : 
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=c" (*rcv_dword1),
     "=S" (src->raw),
     "=D" (tag->raw)
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
l4_ipc_receive_tag(l4_threadid_t src,
               void *rcv_msg, 
               l4_umword_t *rcv_dword0, 
               l4_umword_t *rcv_dword1, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result,
	       l4_msgtag_t *tag)
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
     "=c" (*rcv_dword1),
     "=D" (tag->raw)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "d" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "S" (src.raw)
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

