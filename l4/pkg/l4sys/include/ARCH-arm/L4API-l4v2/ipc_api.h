#ifndef L4_IPC_API_H
#define L4_IPC_API_H

#include <l4/sys/compiler.h>

#ifndef L4_SYSCALL_MAGIC_OFFSET  
#  define L4_SYSCALL_MAGIC_OFFSET 	8
#endif
#define L4_SYSCALL_IPC			(-0x00000004-L4_SYSCALL_MAGIC_OFFSET)

/*
 * Internal defines used to build IPC parameters for the L4 kernel
 */

#define L4_IPC_DECEIT           1
#define L4_IPC_OPEN_IPC         1

#ifdef __GNUC__
/*----------------------------------------------------------------------------
 * 2 words in registers
 *--------------------------------------------------------------------------*/
L4_INLINE int 
l4_ipc_call_tag(l4_threadid_t dest, 
            const void *snd_msg, 
            l4_umword_t snd_w0,
            l4_umword_t snd_w1, 
            l4_msgtag_t tag,
            void *rcv_msg,
            l4_umword_t *rcv_w0, 
            l4_umword_t *rcv_w1,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
            l4_msgtag_t *rtag)
{
  register l4_umword_t _dest     __asm__("r0") = dest.raw;
  register l4_umword_t _snd_desc __asm__("r1") = (l4_umword_t)snd_msg;
  register l4_umword_t _rcv_desc __asm__("r2") = (l4_umword_t)rcv_msg;
  register l4_umword_t _timeout  __asm__("r3") = timeout.raw;
  register l4_umword_t _w0       __asm__("r5") = snd_w0;
  register l4_umword_t _w1       __asm__("r6") = snd_w1;
  register l4_umword_t _tag      __asm__("r4") = tag.raw;
  __asm__ __volatile__ 
    ("@ l4_ipc_call(start) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}    \n\t"
     "mov     lr, pc       \n\t"
     "mov     pc, %7       \n\t"
     "ldmia   sp!, {fp}    \n\t"
     PIC_RESTORE_ASM
     "@ l4_ipc_call(end)   \n\t"
     :
     "=r" (_dest),
     "=r" (_snd_desc),
     "=r" (_rcv_desc),
     "=r" (_timeout),
     "=r" (_w0),
     "=r" (_w1),
     "=r" (_tag)
     : 
     "i" (L4_SYSCALL_IPC),
     "0" (_dest),
     "1" (_snd_desc),
     "2" (_rcv_desc),
     "3" (_timeout),
     "4" (_w0),
     "5" (_w1),
     "6" (_tag)
     : 
     "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory");

  *rcv_w0 = _w0;
  *rcv_w1 = _w1;
  result->raw = _dest;
  rtag->raw = _tag;

  return L4_IPC_ERROR(*result);
}


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
  l4_msgtag_t tag;
  return l4_ipc_call_tag(dest,snd_msg, snd_w0, snd_w1,
      l4_msgtag(0,0,0,0), rcv_msg, rcv_w0, rcv_w1, timeout, result, &tag);
}

L4_INLINE int 
l4_ipc_reply_and_wait_tag(l4_threadid_t dest, 
                      const void *snd_msg, 
                      l4_umword_t snd_w0,
                      l4_umword_t snd_w1, 
		      l4_msgtag_t tag,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_w0, 
                      l4_umword_t *rcv_w1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result,
		      l4_msgtag_t *rtag)
{
  
  register l4_umword_t _dest     __asm__("r0") = dest.raw;
  register l4_umword_t _snd_desc __asm__("r1") = (l4_umword_t)snd_msg;
  register l4_umword_t _rcv_desc __asm__("r2") = (l4_umword_t)rcv_msg | L4_IPC_OPEN_IPC;
  register l4_umword_t _timeout  __asm__("r3") = timeout.raw;
  register l4_umword_t _tag      __asm__("r4") = tag.raw;
  register l4_umword_t _w0       __asm__("r5") = snd_w0;
  register l4_umword_t _w1       __asm__("r6") = snd_w1;

  __asm__ __volatile__ 
    ("@ l4_ipc_reply_and_wait(start) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}              \n\t"
     "mov     lr, pc	               \n\t"
     "mov     pc, %7	               \n\t"
     "ldmia   sp!, {fp}              \n\t"
     PIC_RESTORE_ASM
     "@ l4_ipc_reply_and_wait(end)   \n\t"
     :
     "=r" (_dest),
     "=r" (_snd_desc),
     "=r" (_rcv_desc),
     "=r" (_timeout),
     "=r" (_w0),
     "=r" (_w1),
     "=r" (_tag)
     : 
     "i" (L4_SYSCALL_IPC),
     "0" (_dest),
     "1" (_snd_desc),
     "2" (_rcv_desc),
     "3" (_timeout),
     "4" (_w0),
     "5" (_w1),
     "6" (_tag)
     : 
     "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory");
  *rcv_w0 = _w0;
  *rcv_w1 = _w1;
  src->raw = _snd_desc;
  result->raw = _dest;
  rtag->raw = _tag;
  
  return L4_IPC_ERROR(*result);
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
  l4_msgtag_t tag;
  return l4_ipc_reply_and_wait_tag(dest, snd_msg, snd_w0, snd_w1, 
      l4_msgtag(0,0,0,0),
      src, rcv_msg, rcv_w0, rcv_w1, timeout, result, &tag);
}

L4_INLINE int 
l4_ipc_send_tag(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t w0,
            l4_umword_t w1,
            l4_msgtag_t tag,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  register l4_umword_t _dest     __asm__("r0") = dest.raw;
  register l4_umword_t _snd_desc __asm__("r1") = (l4_umword_t)snd_msg;
  register l4_umword_t _rcv_desc __asm__("r2") = ~0U;
  register l4_umword_t _timeout  __asm__("r3") = timeout.raw;
  register l4_umword_t _w0       __asm__("r5") = w0;
  register l4_umword_t _w1       __asm__("r6") = w1;
  register l4_umword_t _tag	 __asm__("r4") = tag.raw;

  __asm__ __volatile__ 
    ("@  l4_ipc_send(start) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}     \n\t"
     "mov     lr, pc	      \n\t"
     "mov     pc, %7	      \n\t"
     "ldmia   sp!, {fp}     \n\t"
     PIC_RESTORE_ASM
     "@  l4_ipc_send(end)   \n\t"
     :
     "=r" (_dest),
     "=r" (_snd_desc),
     "=r" (_rcv_desc),
     "=r" (_timeout),
     "=r" (_w0),
     "=r" (_w1),
     "=r" (_tag)
     : 
     "i" (L4_SYSCALL_IPC),
     "0" (_dest),
     "1" (_snd_desc),
     "2" (_rcv_desc),
     "3" (_timeout),
     "4" (_w0),
     "5" (_w1),
     "6" (_tag)
     : 
     "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory");
  result->raw = _dest;

  return L4_IPC_ERROR(*result);
}

L4_INLINE int 
l4_ipc_send(l4_threadid_t dest, 
            const void *snd_msg, 
            l4_umword_t w0,
            l4_umword_t w1, 
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  return l4_ipc_send_tag(dest, snd_msg, w0, w1,
      l4_msgtag(0,0,0,0), timeout, result);
}

L4_INLINE int 
l4_ipc_wait_tag(l4_threadid_t *src,
            void *rcv_msg, 
            l4_umword_t *rcv_w0,
            l4_umword_t *rcv_w1,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
            l4_msgtag_t *tag)
{
  
  register l4_umword_t _res      __asm__("r0") = 0;
  register l4_umword_t _snd_desc __asm__("r1") = ~0U;
  register l4_umword_t _rcv_desc __asm__("r2") = (l4_umword_t)rcv_msg | L4_IPC_OPEN_IPC;
  register l4_umword_t _timeout  __asm__("r3") = timeout.raw;
  register l4_umword_t _tag      __asm__("r4");
  register l4_umword_t _w0       __asm__("r5");
  register l4_umword_t _w1       __asm__("r6");

  __asm__ __volatile__
    ("@ l4_ipc_wait(start) \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}    \n\t"
     "mov     lr, pc	     \n\t"
     "mov     pc, %7	     \n\t"
     "ldmia   sp!, {fp}    \n\t"
     PIC_RESTORE_ASM
     "@ l4_ipc_wait(end)   \n\t"
     :
     "=r"(_res),
     "=r"(_snd_desc),
     "=r"(_rcv_desc),
     "=r"(_timeout),
     "=r"(_w0),
     "=r"(_w1),
     "=r"(_tag)
     : 
     "i"(L4_SYSCALL_IPC),
     "0"(_res),
     "1"(_snd_desc),
     "2"(_rcv_desc),
     "3"(_timeout)
     : 
     "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory");
  *rcv_w0     = _w0;
  *rcv_w1     = _w1;
  result->raw	= _res;
  src->raw	= _snd_desc;
  tag->raw = _tag;

  return L4_IPC_ERROR(*result);
}

L4_INLINE int 
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg, 
            l4_umword_t *rcv_w0,
            l4_umword_t *rcv_w1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_msgtag_t tag;
  return l4_ipc_wait_tag(src, rcv_msg, rcv_w0, rcv_w1, timeout, result, &tag);
}

L4_INLINE int 
l4_ipc_receive_tag(l4_threadid_t src,
               void *rcv_msg, 
               l4_umword_t *rcv_w0,
               l4_umword_t *rcv_w1,
               l4_timeout_t timeout,
               l4_msgdope_t *result,
               l4_msgtag_t *tag)
{
  register l4_umword_t _res      __asm__("r0") = src.raw;
  register l4_umword_t _snd_desc __asm__("r1") = ~0U;
  register l4_umword_t _rcv_desc __asm__("r2") = (l4_umword_t)rcv_msg;
  register l4_umword_t _timeout  __asm__("r3") = timeout.raw;
  register l4_umword_t _tag      __asm__("r4");
  register l4_umword_t _w0       __asm__("r5");
  register l4_umword_t _w1       __asm__("r6");

  __asm__ __volatile__
    ("@ l4_ipc_receive(start)  \n\t"
     PIC_SAVE_ASM
     "stmdb   sp!, {fp}        \n\t"
     "mov     lr, pc		 \n\t"
     "mov     pc, %7		 \n\t"
     "ldmia   sp!, {fp}        \n\t"
     PIC_RESTORE_ASM
     "@ l4_ipc_receive(end)    \n\t"
     :		 
     "=r"(_res),
     "=r"(_snd_desc),
     "=r"(_rcv_desc),
     "=r"(_timeout),
     "=r"(_w0),
     "=r"(_w1),
     "=r"(_tag)
     : 
     "i"(L4_SYSCALL_IPC),
     "0"(_res),
     "1"(_snd_desc),
     "2"(_rcv_desc),
     "3"(_timeout)
     : 
     "r7", "r8", "r9" PIC_CLOBBER, "r12", "r14", "memory");
  *rcv_w0 = _w0;
  *rcv_w1 = _w1;
  result->raw	= _res;
  tag->raw = _tag;

  return L4_IPC_ERROR(*result);
}

L4_INLINE int 
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg, 
               l4_umword_t *rcv_w0,
               l4_umword_t *rcv_w1,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  l4_msgtag_t tag;
  return l4_ipc_receive_tag(src, rcv_msg, rcv_w0, rcv_w1, timeout, result,
      &tag);
}

L4_INLINE int
l4_ipc_sleep(l4_timeout_t timeout)
{
  l4_umword_t dummy;
  l4_msgdope_t result;
  return l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
                        timeout,  &result);
}

#endif //__GNUC__


#endif /* L4_IPC_API_H */

