/*!
 * \file  l4sys/include/L4API-l4v2/ipc-impl.h
 * \brief Common IPC inline implementations.
 */
#ifndef __L4SYS__INCLUDE__L4API_FIASCO__IPC_IMPL_H__
#define __L4SYS__INCLUDE__L4API_FIASCO__IPC_IMPL_H__

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
  l4_msgtag_t dummytag;
  return l4_ipc_call_tag(dest, snd_msg, snd_dword0, snd_dword1,
                         l4_msgtag(0,0,0,0), rcv_msg,
                         rcv_dword0, rcv_dword1, timeout, result, &dummytag);
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
  l4_msgtag_t dummytag;
  return l4_ipc_reply_and_wait_tag(dest, snd_msg, snd_dword0, snd_dword1,
                                   l4_msgtag(0,0,0,0), src,
                                   rcv_msg, rcv_dword0, rcv_dword1,
                                   timeout, result, &dummytag);
}

L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_dword0,
            l4_umword_t *rcv_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  l4_msgtag_t dummytag;
  return l4_ipc_wait_tag(src, rcv_msg, rcv_dword0, rcv_dword1, timeout, result,
                         &dummytag);
}

L4_INLINE int
l4_ipc_send(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_dword0,
            l4_umword_t snd_dword1,
            l4_timeout_t timeout,
            l4_msgdope_t *result)
{
  return l4_ipc_send_tag(dest, snd_msg, snd_dword0, snd_dword1,
                         l4_msgtag(0,0,0,0), timeout, result);
}

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_dword0,
               l4_umword_t *rcv_dword1,
               l4_timeout_t timeout,
               l4_msgdope_t *result)
{
  l4_msgtag_t dummytag;
  return l4_ipc_receive_tag(src, rcv_msg, rcv_dword0, rcv_dword1, timeout,
                            result, &dummytag);
}

L4_INLINE int
l4_ipc_sleep(l4_timeout_t timeout)
{
  l4_umword_t dummy;
  l4_msgdope_t result;
  return l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
                        timeout, &result);
}

L4_INLINE int
l4_ipc_fpage_received(l4_msgdope_t msgdope)
{
  return msgdope.md.fpage_received != 0;
}

L4_INLINE int
l4_ipc_is_fpage_granted(l4_fpage_t fp)
{
  return fp.fp.grant != 0;
}

L4_INLINE int
l4_ipc_is_fpage_writable(l4_fpage_t fp)
{
  return fp.fp.write != 0;
}

L4_INLINE long
l4_is_long_rcv_descr(const void *msg)
{
  if (!l4_is_rcv_map_descr(msg))
    return (long)msg & ~0x03;
  else
    return 0;
}

L4_INLINE long
l4_is_rcv_map_descr(const void *msg)
{
  return (long)msg & 0x2;
}

L4_INLINE long
l4_is_long_snd_descr(const void *msg)
{
  return (long)msg & ~0x03;
}

L4_INLINE const l4_msg_t *
l4_get_snd_msg_from_descr(const void *msg)
{
  return (const l4_msg_t *)((l4_addr_t)msg & ~0x03);
}

L4_INLINE l4_msg_t *
l4_get_rcv_msg_from_descr(void *msg)
{
  return (l4_msg_t*)((l4_addr_t)msg & ~0x03);
}

#endif /* ! __L4SYS__INCLUDE__L4API_FIASCO__IPC_IMPL_H__ */
