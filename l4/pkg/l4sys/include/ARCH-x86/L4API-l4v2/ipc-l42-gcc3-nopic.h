/*
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC3_NOPIC_H__
#define __L4_IPC_L42_GCC3_NOPIC_H__


L4_INLINE int
l4_ipc_call_tag(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
	    l4_msgtag_t snd_tag,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
	    l4_msgtag_t *rcv_tag)
{
  struct { unsigned dummy1, dummy2; } v = {dest.raw, snd_tag.raw};

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %[rcv_desc], %%ebp		\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (v.dummy1),
     "=S" (v.dummy2),
     "=D" (rcv_tag->raw)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "c" (timeout),
     "S" (&v),
     [rcv_desc] "ir"(((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
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
  struct { unsigned d,v; } v = {dest.raw, tag.raw};

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %[msg_desc], %%ebp		\n\t"
     "movl  4(%%esi), %%edi \n\t"
     "movl  (%%esi), %%esi  \n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (v.d),
     "=S" (src->raw),
     "=D" (rtag->raw)
     :
     "a" ((int)snd_msg),
     "d" (snd_dword0),
     "b" (snd_dword1),
     "c" (timeout),
     "S" (&v),
     [msg_desc] "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC)
     :
     "memory"
     );


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
     "S" (dest.raw),
     "D" (tag.raw)
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
  unsigned dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
     "movl  %[msg_desc],%%ebp	\n\t" /* rcv_msg */
     "xorl  %%edi,%%edi	\n\t" /* no absolute timeout !! */
     IPC_SYSENTER
     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (src->raw),
     "=D" (tag->raw)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     [msg_desc] "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC)
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
  unsigned dummy, tag;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
     "movl  %[msg_desc], %%ebp	\n\t" /* rcv_msg */
     IPC_SYSENTER
     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (src->raw),
     "=D" (tag)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     [msg_desc] "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC),
     "D" (L4_IPC_FLAG_NEXT_PERIOD) /* no absolute timeout */
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
  unsigned dummy;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %[msg_desc],%%ebp		\n\t"
     IPC_SYSENTER
     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=D" (tag->raw)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "S" (src.raw),
     [msg_desc] "ir"(((int)rcv_msg) & (~L4_IPC_OPEN_IPC))
     );
  return L4_IPC_ERROR(*result);
}

#endif

