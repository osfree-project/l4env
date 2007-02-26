/* 
 * $Id$
 */

#ifndef __L4_IPC_L42_GCC273_PIC_H__ 
#define __L4_IPC_L42_GCC273_PIC_H__ 

/*
 * Implementation
 */

#define SCRATCH 1
#define SCRATCH_MEMORY 1

L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, 
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  struct {
    l4_umword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     "pushl	%%ebx		\n\t"  
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"

	     "movl	%%edi, %%ebp	\n\t"

	     "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> edi */

	     "int	$0x30		\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
	     : 
	     "=a" (*result),				/* EAX, 0 	*/
	     "=d" (*rcv_dword0),			/* EDX, 1 	*/
	     "=c" (*rcv_dword1)				/* ECX, 2 	*/
	     :
	     "D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)), /* EDI, 3, rcv msg -> ebp */
	     "S" (&dest),			        /* ESI, 4, addr of dest	  */
	     "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0  	*/
	     "1" (&dwords),				/* EDX, 1,	*/
	     "2" (timeout)				/* ECX, 2 	*/
#ifdef SCRATCH
	     :
	     "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	     , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	     );
  return L4_IPC_ERROR(*result);
}



L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
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
    l4_threadid_t *src;
  } addresses = { &dest, src };

  struct {
    l4_umword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     /* eax, edx, ebx loaded, 
	      * edi contains rcv buffer address, must be moved to ebp,
	      * esi contains address of destination id,
	      * $5  address of src id
	      */
	     "pushl	%%ebx		\n\t"  
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"

	     "pushl	%%esi		\n\t"
	     "movl	(%%esi), %%esi	\n\t"		/* load address of dest */
	     
	     "movl	%%edi, %%ebp	\n\t"
	     "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> esi */
	     "int	$0x30		\n\t"
	     "popl	%%ebp		\n\t"
	     "movl	4(%%ebp), %%ebp	\n\t"		/* load address of src */
	     "movl	%%esi, (%%ebp)  \n\t"		/* esi -> src.lh.low  */
	     "movl	%%edi, 4(%%ebp)\n\t"		/* edi -> src.lh.high */

	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
	     
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=c" (*rcv_dword1)			/* ECX, 2 	*/
      :
      "2" (timeout),				/* ECX, 3	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* edi, 4  -> ebp rcv_msg */
      "S" (&addresses),				/* ESI ,5	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (&dwords)				/* EDX, 1 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_reply_deceiting_and_wait(l4_ipc_deceit_ids_t ids,
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
    l4_ipc_deceit_ids_t *ids;
    l4_threadid_t *src;
  } addresses = { &ids, src };

  struct {
    l4_umword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     /* eax, edx, ebx loaded, 
	      * edi contains rcv buffer address, must be moved to ebp,
	      * esi contains address of destination id,
	      * esi+8 is the address of true src
	      * $5  address of src id
	      */
	     "pushl	%%ebx		\n\t"	
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl	%%esi		\n\t"
	     "movl	(%%esi), %%esi	\n\t"		/* load address of ids */

	     "movl	%%edi, %%ebp	\n\t"
	     "pushl	12(%%esi)	\n\t"		/* ids.true_src.lh.high */
	     /*	->(esp+4) */
	     "pushl	8(%%esi)	\n\t"		/* ids.true_src.lh.low  */ 
	     /*   	->(esp)   */
	     "movl	4(%%esi), %%edi	\n\t"		/* ids.dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* ids.dest.lh.low  -> edi */
	     
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"
	     

	     "int	$0x30		\n\t"
	     "addl	$8, %%esp	\n\t"		/* remove true_src from stack*/

	     "popl	%%ebp		\n\t"
	     "movl	4(%%ebp), %%ebp	\n\t"		/* load address of src */
	     "movl	%%esi, (%%ebp)  \n\t"		/* esi -> src.lh.low  */
	     "movl	%%edi, 4(%%ebp)\n\t"		/* edi -> src.lh.high */
	     
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=c" (*rcv_dword1)			/* ECX, 2 	*/
      :
      "S" (&addresses),				/* addresses, 3	*/
      "2" (timeout),				/* ECX, 2	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* EDI, 4 -> ebp rcv_msg */
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (dwords)				/* EDX, 1 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int 
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, 
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  asm 
    volatile(
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl    %%ebx		\n\t"
	     "movl	%5 ,%%ebp	\n\t"
	     "movl	%%esi, %%ebx	\n\t"
	     "movl	(%%edi),%%esi	\n\t"
	     "movl	4(%%edi),%%edi	\n\t"
	     "int	$0x30		\n\t"
	     "popl	%%ebx		\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     : 
	     "=a" (*result)				/* EAX,0 	*/
	     :
	     "d" (snd_dword0),				/* EDX, 1	*/
	     "c" (timeout),				/* ECX, 2	*/
	     "S" (snd_dword1),				/* EBX, 3	*/
	     "D" (&dest),				/* EDI, 4	*/
	     "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 5 	*/
	     "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
	     :
	     "esi", "edi", "ecx", "edx"
#ifdef SCRATCH_MEMORY
	     , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	     );
  return L4_IPC_ERROR(*result);
};

L4_INLINE int
l4_i386_ipc_send_deceiting(l4_ipc_deceit_ids_t ids,
			   const void *snd_msg, 
			   l4_umword_t snd_dword0, 
			   l4_umword_t snd_dword1, 
			   l4_timeout_t timeout, 
			   l4_msgdope_t *result)
{
  struct {
    l4_umword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     "pushl	%%ebx		\n\t"  

	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"
		     
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl	4(%%esi)	\n\t"		
	     "pushl	(%%esi)		\n\t"		
	     "movl	(%%edi), %%esi	\n\t"
	     "movl	4(%%edi), %%edi	\n\t"
	     
	     "movl	%6 ,%%ebp	\n\t" 
	     
	     "int	$0x30		\n\t"

	     /* remove true_src from stack */
	     "addl	$8, %%esp	\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "popl	%%ebx		\n\t"
      : 
      "=a" (*result)				/* EAX,0 	*/
      :
      "d" (&dwords),				/* EDX, 1	*/
      "c" (timeout),				/* ECX, 2	*/
      "D" (&ids.dest),				/* EDI, 4	*/
      "S" (&ids.true_src),			/* ESI, 5	*/
      "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 6 	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ebx", "ecx", "edx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
};

L4_INLINE int 
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, 
		    l4_umword_t *rcv_dword0, 
		    l4_umword_t *rcv_dword1, 
		    l4_timeout_t timeout, 
		    l4_msgdope_t *result)
{
  asm
    volatile (
	      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	      "pushl	%%ebx		\n\t"
	      "movl	%2,%%ebp	\n\t"
	      "movl	(%%edi),%%esi	\n\t"
	      "movl	4(%%edi),%%edi	\n\t" 
	      "int	$0x30		\n\t"
	      "movl	%%ebx, %2	\n\t"
	      "popl	%%ebx		\n\t"
	      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	      : 
	      "=a" (*result),				/* EAX,0 */
	      "=d" (*rcv_dword0),			/* EDX,1 */
	      "=S" (*rcv_dword1)		       	/* ESI,2 */
	      :
	      "c" (timeout),				/* ECX, 3 	*/
	      "D" (&src),		       		/* EDI, 4 	*/
	      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* ESI, 2, rcv_msg -> EBP */
#ifdef SCRATCH
	      :
	      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	      );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int 
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  asm
    volatile (
	      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	      "pushl	%%ebx		\n\t"
	      "movl	%%esi,%%ebp	\n\t"
	      "pushl	%%edi		\n\t"
	      "int	$0x30		\n\t"
	      "popl	%%ebp		\n\t"
	      "movl	%%esi,(%%ebp)	\n\t"
	      "movl	%%edi,4(%%ebp)	\n\t" 
	      "movl	%%ebx,%%esi	\n\t"
	      "popl	%%ebx		\n\t"
	      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	      : 
	      "=a" (*result),				/* EAX,0 */
	      "=d" (*rcv_dword0),			/* EDX,1 */
	      "=S" (*rcv_dword1)		       	/* ESI,2 */
	      :
	      "c" (timeout),				/* ECX, 3 	*/
	      "D" (src),		       		/* EDI, 4 	*/
	      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* ESI, 2, rcv_msg -> EBP */
#ifdef SCRATCH
	      :
	      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	      );
  return L4_IPC_ERROR(*result);
}

#endif /* __L4_IPC__ */
