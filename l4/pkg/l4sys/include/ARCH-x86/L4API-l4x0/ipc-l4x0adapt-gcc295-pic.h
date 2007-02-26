/* 
 * $Id$
 */

#ifndef __L4_IPC_L4X0_GCC295_PIC_H__ 
#define __L4_IPC_L4X0_GCC295_PIC_H__


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
  l4_umword_t dummy1, dummy2;
  struct 
  {
    l4_umword_t d1, d2;
  } dwords = { snd_dword0, snd_dword1 };

  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  4(%%edx),%%ebx	\n\t"
     "movl   (%%edx),%%edx	\n\t"
     "movl   %%edi, %%ebp	\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongOut

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx	\n\t"
     "popl  %%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy1),
     "=D" (dummy2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dwords),
     "S" (&dest),
     "D" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC))
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
  l4_umword_t dummy;
  l4_umword_t dw[3] = { snd_dword0, snd_dword1, snd_dword2 };

  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %%edi, %%ebp	\n\t"

     "movl  8(%%edx),%%edi	\n\t"
     "movl  4(%%edx),%%ebx	\n\t"
     "movl   (%%edx),%%edx	\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "popl  %%ebx		\n\t"
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
  l4_umword_t dummy1, dummy2;

  struct {
    l4_threadid_t *dest;
    l4_threadid_t *src;
  } addresses = { &dest, src };
  struct {
    l4_umword_t d1, d2;
  } dwords = { snd_dword0, snd_dword1 };

  __asm__ __volatile__
    ("pushl  %%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  4(%%edx),%%ebx	\n\t"
     "movl   (%%edx),%%edx	\n\t"

     "pushl  %%esi		\n\t"
     "movl   (%%esi), %%esi	\n\t"

     "movl  %%edi, %%ebp	\n\t"
     "movl  4(%%esi),%%edi	\n\t"
     "movl   (%%esi),%%esi	\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongOut
     FromId32_Esi
                      
     "popl  %%ebp		\n\t"
     "movl 4(%%ebp),%%ebp	\n\t"
     "movl  %%esi,(%%ebp)	\n\t"
     "movl  %%edi,4(%%ebp)	\n\t"

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl  %%ebx,%%ecx		\n\t"
     "popl  %%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy1),
     "=D" (dummy2)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dwords),
     "S" (&addresses),
     "D" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC)
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
  l4_umword_t src32;
  l4_umword_t dw[3] = { snd_dword0, snd_dword1, snd_dword2 };

  __asm__ __volatile__
    ("pushl  %%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %%edi, %%ebp	\n\t"

     "movl  8(%%edx),%%edi	\n\t"
     "movl  4(%%edx),%%ebx	\n\t"
     "movl   (%%edx),%%edx	\n\t"

     IPC_SYSENTER

     "popl   %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "movl   %%ebx,%%ecx	\n\t"
     "popl   %%ebx		\n\t"
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
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %%edi,%%ebx		\n\t"
     "movl 4(%%esi),%%edi	\n\t"
     "movl  (%%esi),%%esi	\n\t"
     "orl   $-1,%%ebp		\n\t"

     ToId32_EdiEsi
     FixLongIn

     IPC_SYSENTER

     FixLongStackOut

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     "popl  %%ebx		\n\t"
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
     "S" (&dest),
     "D" (snd_dword1)
     :
     "memory"
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
  l4_umword_t dummy1, dummy2, dummy3, dummy4;
  l4_umword_t dw[2] = { snd_dword0, snd_dword1 };

  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
     "movl  %%edi,%%ebx		\n\t"
     "movl 4(%%edx),%%ebx	\n\t"
     "movl  (%%edx),%%edx	\n\t"
     "orl   $-1,%%ebp		\n\t"

     IPC_SYSENTER

     "popl  %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
     "popl  %%ebx		\n\t"
     :
     "=a" (*result),
     "=c" (dummy1),
     "=d" (dummy2),
     "=S" (dummy3),
     "=D" (dummy4)
     :
     "a" ((l4_umword_t)snd_msg),
     "c" (timeout),
     "d" (&dw[0]),
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
l4_ipc_wait(l4_threadid_t * src,
            void * rcv_msg, 
            l4_umword_t * rcv_dword0, 
            l4_umword_t * rcv_dword1, 
            l4_timeout_t timeout, 
            l4_msgdope_t * result)
{
  __asm__ __volatile__
    ("pushl %%ebx		\n\t"
     "pushl %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl  %%edx,%%ebp		\n\t"

     FixLongStackIn

     IPC_SYSENTER

     FixLongOut
     FromId32_Esi

     "movl  %%ebx,%%ecx		\n\t"
     "popl  %%ebp		\n\t"	/* restore ebp, no memory
					   references ("m") before this point */
     "popl  %%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src->lh.low),
     "=D" (src->lh.high)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "d" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "S" (0)			      /* no absolute timeout !! */
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_wait_w3(l4_threadid_t * src,
               void * rcv_msg, 
      	       l4_umword_t * rcv_dword0, 
	       l4_umword_t * rcv_dword1, 
	       l4_umword_t * rcv_dword2, 
	       l4_timeout_t timeout, 
	       l4_msgdope_t * result)
{
  l4_umword_t src32;

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
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (src32),
     "=D" (*rcv_dword2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "d" (((l4_umword_t)rcv_msg) | L4_IPC_OPEN_IPC),
     "S" (0)			      /* no absolute timeout !! */
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
    ("pushl  %%ebx		\n\t"
     "pushl  %%ebp		\n\t"	/* save ebp, no memory references 
					   ("m") after this point */
     "movl   %%edx,%%ebp	\n\t"

     ToId32_EdiEsi
     FixLongStackIn

     IPC_SYSENTER

     FixLongOut

     "movl   %%ebx,%%ecx	\n\t"
     "popl   %%ebp		\n\t"	/* restore ebp, no memory references 
					   ("m") before this point */
     "popl   %%ebx		\n\t"
     : 
     "=a" (*result),
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy1),
     "=D" (dummy2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "d" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "S" (src.lh.low),
     "D" (src.lh.high)
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
  l4_umword_t dummy;

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
     "=c" (*rcv_dword1),
     "=d" (*rcv_dword0),
     "=S" (dummy),
     "=D" (*rcv_dword2)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     "d" (((l4_umword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)),
     "S" (l4sys_to_id32(src))
     :
     "memory"
     );
  return L4_IPC_ERROR(*result);
}

#endif

