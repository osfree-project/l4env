#ifndef L4_IPC_API_H
#define L4_IPC_API_H

#ifndef L4_SYSCALL_MAGIC_OFFSET  
#  define L4_SYSCALL_MAGIC_OFFSET 	8
#endif
#define L4_SYSCALL_IPC			(-0x00000004-L4_SYSCALL_MAGIC_OFFSET)

/*----------------------------------------------------------------------------
 * 2 words in registers
 *--------------------------------------------------------------------------*/
L4_INLINE int 
l4_ipc_call(l4_threadid_t dest, 
            const void *snd_msg, 
            l4_umword_t snd_w0,
            l4_umword_t snd_w1, 
            void *rcv_msg,
            l4_umword_t *rcv_w0, 
            l4_umword_t *rcv_w1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  register l4_umword_t _w2       asm("r6") = 0;

  if(l4_is_long_snd_descr(snd_msg)) 
    {
      const l4_msg_t *msg = l4_get_snd_msg_from_descr(snd_msg);
      if(msg->send.md.words >=3)
        _w2 = msg->word[2];
    }
  {
    register l4_umword_t _dest     asm("r0") = dest.raw;
    register l4_umword_t _snd_desc asm("r1") = (l4_umword_t)snd_msg;
    register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg;
    register l4_umword_t _timeout  asm("r3") = timeout.raw;
    register l4_umword_t _w0       asm("r4") = snd_w0;
    register l4_umword_t _w1       asm("r5") = snd_w1;
    __asm__ __volatile__ 
      ("@ l4_ipc_call(start) \n\t"
       "stmdb sp!, {fp}      \n\t"
       "mov	lr, pc			     \n\t"
       "mov	pc, %7			     \n\t"
       "ldmia sp!, {fp}      \n\t"
       "@ l4_ipc_call(end)   \n\t"
       :
       "=r" (_dest),
       "=r" (_snd_desc),
       "=r" (_rcv_desc),
       "=r" (_timeout),
       "=r" (_w0),
       "=r" (_w1),
       "=r" (_w2)
       : 
       "i" (L4_SYSCALL_IPC),
       "0" (_dest),
       "1" (_snd_desc),
       "2" (_rcv_desc),
       "3" (_timeout),
       "4" (_w0),
       "5" (_w1),
       "6" (_w2)
       : 
       "r7", "r8", "r9", "r10", "r12", "r14", "memory"
       );
    *rcv_w0 = _w0;
    *rcv_w1 = _w1;
    result->raw = _dest;
  }
  if(l4_is_long_rcv_descr(rcv_msg)) 
    {
      l4_msg_t *msg = l4_get_rcv_msg_from_descr(rcv_msg);
      if(msg->size.md.words >=3)
        msg->word[2] = _w2;
    }

  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_reply_and_wait(l4_threadid_t dest, 
                      const void *snd_msg, 
                      l4_umword_t snd_w0,
                      l4_umword_t snd_w1, 
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_w0, 
                      l4_umword_t *rcv_w1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result)
{
  
  register l4_umword_t _w2       asm("r6") = 0;

  if(l4_is_long_snd_descr(snd_msg)) 
    {
      const l4_msg_t *msg = l4_get_snd_msg_from_descr(snd_msg);
      if(msg->send.md.words >=3)
        _w2 = msg->word[2];
    }
  {
    register l4_umword_t _dest     asm("r0") = dest.raw;
    register l4_umword_t _snd_desc asm("r1") = (l4_umword_t)snd_msg;
    register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg | L4_IPC_OPEN_IPC;
    register l4_umword_t _timeout  asm("r3") = timeout.raw;
    register l4_umword_t _w0       asm("r4") = snd_w0;
    register l4_umword_t _w1       asm("r5") = snd_w1;
    
    __asm__ __volatile__ 
      ("@ l4_ipc_reply_and_wait(start) \n\t"
       "stmdb sp!, {fp}                \n\t"
       "mov	lr, pc			               \n\t"
       "mov	pc, %7			               \n\t"
       "ldmia sp!, {fp}                \n\t"
       "@ l4_ipc_reply_and_wait(end)   \n\t"
       :
       "=r" (_dest),
       "=r" (_snd_desc),
       "=r" (_rcv_desc),
       "=r" (_timeout),
       "=r" (_w0),
       "=r" (_w1),
       "=r" (_w2)
       : 
       "i" (L4_SYSCALL_IPC),
       "0" (_dest),
       "1" (_snd_desc),
       "2" (_rcv_desc),
       "3" (_timeout),
       "4" (_w0),
       "5" (_w1),
       "6" (_w2)
       : 
       "r7", "r8", "r9", "r10", "r12", "r14", "memory"
       );
    *rcv_w0 = _w0;
    *rcv_w1 = _w1;
    src->raw = _snd_desc;
    result->raw = _dest;
  }

  if(l4_is_long_rcv_descr(rcv_msg)) 
    {
      l4_msg_t *msg = l4_get_rcv_msg_from_descr(rcv_msg);
      if(msg->size.md.words >=3)
        msg->word[2] = _w2;
    }
  
  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_send(l4_threadid_t dest, 
            const void *snd_msg, 
            l4_umword_t w0,
            l4_umword_t w1, 
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  
  register l4_umword_t _w2       asm("r6") = 0;

  if(l4_is_long_snd_descr(snd_msg)) 
    {
      const l4_msg_t *msg = l4_get_snd_msg_from_descr(snd_msg);
      if(msg->send.md.words >=3)
        _w2 = msg->word[2];
    }
  {
    register l4_umword_t _dest     asm("r0") = dest.raw;
    register l4_umword_t _snd_desc asm("r1") = (l4_umword_t)snd_msg;
    register l4_umword_t _rcv_desc asm("r2") = ~0U;
    register l4_umword_t _timeout  asm("r3") = timeout.raw;
    register l4_umword_t _w0       asm("r4") = w0;
    register l4_umword_t _w1       asm("r5") = w1;
    
    __asm__ __volatile__ 
      ("@  l4_ipc_send(start) \n\t"
       "stmdb sp!, {fp}       \n\t"
       "mov	lr, pc			      \n\t"
       "mov	pc, %7			      \n\t"
       "ldmia sp!, {fp}       \n\t"
       "@  l4_ipc_send(end)   \n\t"
       :
       "=r" (_dest),
       "=r" (_snd_desc),
       "=r" (_rcv_desc),
       "=r" (_timeout),
       "=r" (_w0),
       "=r" (_w1),
       "=r" (_w2)
       : 
       "i" (L4_SYSCALL_IPC),
       "0" (_dest),
       "1" (_snd_desc),
       "2" (_rcv_desc),
       "3" (_timeout),
       "4" (_w0),
       "5" (_w1),
       "6" (_w2)
       : 
       "r7", "r8", "r9", "r10", "r12", "r14", "memory"
       );
    result->raw = _dest;
  }

  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg, 
            l4_umword_t *rcv_w0,
            l4_umword_t *rcv_w1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  
  register l4_umword_t _w2       asm("r6");

  {
    register l4_umword_t _res      asm("r0");
    register l4_umword_t _snd_desc asm("r1") = ~0U;
    register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg | L4_IPC_OPEN_IPC;
    register l4_umword_t _timeout  asm("r3") = timeout.raw;
    register l4_umword_t _w0       asm("r4");
    register l4_umword_t _w1       asm("r5");
    
    __asm__ __volatile__
      ("@ l4_ipc_wait(start) \n\t"
       "stmdb sp!, {fp}      \n\t"
       "mov	lr, pc		       \n\t"
       "mov	pc, %7		       \n\t"
       "ldmia sp!, {fp}      \n\t"
       "@ l4_ipc_wait(end)	 \n\t"
       :
       "=r"(_res),
       "=r"(_snd_desc),
       "=r"(_rcv_desc),
       "=r"(_timeout),
       "=r"(_w0),
       "=r"(_w1),
       "=r"(_w2)
       : 
       "i"(L4_SYSCALL_IPC),
       "1"(_snd_desc),
       "2"(_rcv_desc),
       "3"(_timeout)
       : 
       "r7", "r8", "r9", "r10", "r12", "r14", "memory"
       );
    *rcv_w0     = _w0;
    *rcv_w1     = _w1;
    result->raw	= _res;
    src->raw	= _snd_desc;
  }

  if(l4_is_long_rcv_descr(rcv_msg)) 
    {
      l4_msg_t *msg = l4_get_rcv_msg_from_descr(rcv_msg);
      if(msg->size.md.words >=3)
        msg->word[2] = _w2;
    }
  
  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg, 
               l4_umword_t *rcv_w0,
               l4_umword_t *rcv_w1,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  register l4_umword_t _w2       asm("r6");
  {
    register l4_umword_t _res      asm("r0") = src.raw;
    register l4_umword_t _snd_desc asm("r1") = ~0U;
    register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg;
    register l4_umword_t _timeout  asm("r3") = timeout.raw;
    register l4_umword_t _w0       asm("r4");
    register l4_umword_t _w1       asm("r5");
 
    __asm__ __volatile__
      ("@ l4_ipc_receive(start)  \n\t"
       "stmdb sp!, {fp}          \n\t"
       "mov	lr, pc		           \n\t"
       "mov	pc, %7		           \n\t"
       "ldmia sp!, {fp}          \n\t"
       "@ l4_ipc_receive(end)    \n\t"
       :		 
       "=r"(_res),
       "=r"(_snd_desc),
       "=r"(_rcv_desc),
       "=r"(_timeout),
       "=r"(_w0),
       "=r"(_w1),
       "=r"(_w2)
       : 
       "i"(L4_SYSCALL_IPC),
       "0"(_res),
       "1"(_snd_desc),
       "2"(_rcv_desc),
       "3"(_timeout)
       : 
       "r7", "r8", "r9", "r10", "r12", "r14", "memory"
       );
    *rcv_w0 = _w0;
    *rcv_w1 = _w1;
    result->raw	= _res;
  }
  
  if(l4_is_long_rcv_descr(rcv_msg)) 
    {
      l4_msg_t *msg = l4_get_rcv_msg_from_descr(rcv_msg);
      if(msg->size.md.words >=3)
        msg->word[2] = _w2;
    }

  return result->md.error_code;
}

/*----------------------------------------------------------------------------
 * 3 words in registers
 *--------------------------------------------------------------------------*/
L4_INLINE int 
l4_ipc_call_w3(l4_threadid_t dest, 
               const void *snd_msg, 
               l4_umword_t snd_w0,
               l4_umword_t snd_w1, 
               l4_umword_t snd_w2,
               void *rcv_msg,
               l4_umword_t *rcv_w0, 
               l4_umword_t *rcv_w1,
               l4_umword_t *rcv_w2,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  register l4_umword_t _dest     asm("r0") = dest.raw;
  register l4_umword_t _snd_desc asm("r1") = (l4_umword_t)snd_msg;
  register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg;
  register l4_umword_t _timeout  asm("r3") = timeout.raw;
  register l4_umword_t _w0       asm("r4") = snd_w0;
  register l4_umword_t _w1       asm("r5") = snd_w1;
  register l4_umword_t _w2       asm("r6") = snd_w2;
  
  __asm__ __volatile__ 
    ("@ l4_ipc_call(start) \n\t"
     "stmdb sp!, {fp}      \n\t"
     "mov	lr, pc			     \n\t"
     "mov	pc, %7			     \n\t"
     "ldmia sp!, {fp}      \n\t"
     "@ l4_ipc_call(end)   \n\t"
     :
     "=r" (_dest),
     "=r" (_snd_desc),
     "=r" (_rcv_desc),
     "=r" (_timeout),
     "=r" (_w0),
     "=r" (_w1),
     "=r" (_w2)
     : 
     "i" (L4_SYSCALL_IPC),
     "0" (_dest),
     "1" (_snd_desc),
     "2" (_rcv_desc),
     "3" (_timeout),
     "4" (_w0),
     "5" (_w1),
     "6" (_w2)
     : 
     "r7", "r8", "r9", "r10", "r12", "r14", "memory"
     );
  *rcv_w0 = _w0;
  *rcv_w1 = _w1;
  *rcv_w2 = _w2;
  result->raw = _dest;
  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_reply_and_wait_w3(l4_threadid_t dest, 
                         const void *snd_msg, 
                         l4_umword_t snd_w0,
                         l4_umword_t snd_w1, 
                         l4_umword_t snd_w2,
                         l4_threadid_t *src,
                         void *rcv_msg,
                         l4_umword_t *rcv_w0, 
                         l4_umword_t *rcv_w1,
                         l4_umword_t *rcv_w2,
                         l4_timeout_t timeout,
                         l4_msgdope_t *result)
{
  register l4_umword_t _dest     asm("r0") = dest.raw;
  register l4_umword_t _snd_desc asm("r1") = (l4_umword_t)snd_msg;
  register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg | L4_IPC_OPEN_IPC;
  register l4_umword_t _timeout  asm("r3") = timeout.raw;
  register l4_umword_t _w0       asm("r4") = snd_w0;
  register l4_umword_t _w1       asm("r5") = snd_w1;
  register l4_umword_t _w2       asm("r6") = snd_w2;
	
  __asm__ __volatile__ 
    ("@ l4_ipc_reply_and_wait(start) \n\t"
     "stmdb sp!, {fp}                \n\t"
     "mov	lr, pc			               \n\t"
     "mov	pc, %7			               \n\t"
     "ldmia sp!, {fp}                \n\t"
     "@ l4_ipc_reply_and_wait(end)   \n\t"
     :
     "=r" (_dest),
     "=r" (_snd_desc),
     "=r" (_rcv_desc),
     "=r" (_timeout),
     "=r" (_w0),
     "=r" (_w1),
     "=r" (_w2)
     : 
     "i" (L4_SYSCALL_IPC),
     "0" (_dest),
     "1" (_snd_desc),
     "2" (_rcv_desc),
     "3" (_timeout),
     "4" (_w0),
     "5" (_w1),
     "6" (_w2)
     : 
     "r7", "r8", "r9", "r10", "r12", "r14", "memory"
     );
  *rcv_w0 = _w0;
  *rcv_w1 = _w1;
  *rcv_w2 = _w2;
  src->raw = _snd_desc;
  result->raw = _dest;
  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_send_w3(l4_threadid_t dest, 
               const void *snd_msg, 
               l4_umword_t w0,
               l4_umword_t w1, 
               l4_umword_t w2,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  
  register l4_umword_t _dest     asm("r0") = dest.raw;
  register l4_umword_t _snd_desc asm("r1") = (l4_umword_t)snd_msg;
  register l4_umword_t _rcv_desc asm("r2") = ~0U;
  register l4_umword_t _timeout  asm("r3") = timeout.raw;
  register l4_umword_t _w0       asm("r4") = w0;
  register l4_umword_t _w1       asm("r5") = w1;
  register l4_umword_t _w2       asm("r6") = w2;
  
  __asm__ __volatile__ 
    ("@  l4_ipc_send(start) \n\t"
     "stmdb sp!, {fp}       \n\t"
     "mov	lr, pc			      \n\t"
     "mov	pc, %7			      \n\t"
     "ldmia sp!, {fp}       \n\t"
     "@  l4_ipc_send(end)   \n\t"
     :
     "=r" (_dest),
     "=r" (_snd_desc),
     "=r" (_rcv_desc),
     "=r" (_timeout),
     "=r" (_w0),
     "=r" (_w1),
     "=r" (_w2)
     : 
     "i" (L4_SYSCALL_IPC),
     "0" (_dest),
     "1" (_snd_desc),
     "2" (_rcv_desc),
     "3" (_timeout),
     "4" (_w0),
     "5" (_w1),
     "6" (_w2)
     : 
     "r7", "r8", "r9", "r10", "r12", "r14", "memory"
     );
  result->raw = _dest;
  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_wait_w3(l4_threadid_t *src,
               void *rcv_msg, 
               l4_umword_t *rcv_w0,
               l4_umword_t *rcv_w1,
               l4_umword_t *rcv_w2,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  register l4_umword_t _res      asm("r0");
  register l4_umword_t _snd_desc asm("r1") = ~0U;
  register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg | L4_IPC_OPEN_IPC;
  register l4_umword_t _timeout  asm("r3") = timeout.raw;
  register l4_umword_t _w0       asm("r4");
  register l4_umword_t _w1       asm("r5");
  register l4_umword_t _w2       asm("r6");
  
  __asm__ __volatile__
    ("@ l4_ipc_wait(start) \n\t"
     "stmdb sp!, {fp}      \n\t"
     "mov	lr, pc		       \n\t"
     "mov	pc, %7		       \n\t"
     "ldmia sp!, {fp}      \n\t"
     "@ l4_ipc_wait(end)	 \n\t"
     :
     "=r"(_res),
     "=r"(_snd_desc),
     "=r"(_rcv_desc),
     "=r"(_timeout),
     "=r"(_w0),
     "=r"(_w1),
     "=r"(_w2)
     : 
     "i"(L4_SYSCALL_IPC),
     "1"(_snd_desc),
     "2"(_rcv_desc),
     "3"(_timeout)
     : 
     "r7", "r8", "r9", "r10", "r12", "r14", "memory"
     );
  *rcv_w0     = _w0;
  *rcv_w1     = _w1;
  *rcv_w2     = _w2;
  result->raw	= _res;
  src->raw	  = _snd_desc;
  
  return result->md.error_code;
}

L4_INLINE int 
l4_ipc_receive_w3(l4_threadid_t src,
                  void *rcv_msg, 
                  l4_umword_t *rcv_w0,
                  l4_umword_t *rcv_w1,
                  l4_umword_t *rcv_w2,
                  l4_timeout_t timeout,
                  l4_msgdope_t *result)
{
  register l4_umword_t _res      asm("r0") = src.raw;
  register l4_umword_t _snd_desc asm("r1") = ~0U;
  register l4_umword_t _rcv_desc asm("r2") = (l4_umword_t)rcv_msg;
  register l4_umword_t _timeout  asm("r3") = timeout.raw;
  register l4_umword_t _w0       asm("r4");
  register l4_umword_t _w1       asm("r5");
  register l4_umword_t _w2       asm("r6");
  
  __asm__ __volatile__
    ("@ l4_ipc_receive(start)  \n\t"
     "stmdb sp!, {fp}          \n\t"
     "mov	lr, pc		           \n\t"
     "mov	pc, %7		           \n\t"
     "ldmia sp!, {fp}          \n\t"
     "@ l4_ipc_receive(end)    \n\t"
     :		 
     "=r"(_res),
     "=r"(_snd_desc),
     "=r"(_rcv_desc),
     "=r"(_timeout),
     "=r"(_w0),
     "=r"(_w1),
     "=r"(_w2)
     : 
     "i"(L4_SYSCALL_IPC),
     "0"(_res),
     "1"(_snd_desc),
     "2"(_rcv_desc),
     "3"(_timeout)
     : 
     "r7", "r8", "r9", "r10", "r12", "r14", "memory"
     );
  *rcv_w0 = _w0;
  *rcv_w1 = _w1;
  *rcv_w2 = _w2;
  result->raw	= _res;
  return result->md.error_code;
}


#endif /* L4_IPC_API_H */
