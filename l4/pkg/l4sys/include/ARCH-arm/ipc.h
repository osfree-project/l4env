/*
 * $Id$
 */

#ifndef L4_IPC_H
#define L4_IPC_H

/*
 * L4 ipc
 */

#include <l4/sys/types.h>
#include <l4/sys/consts_ipc.h>
#include <l4/sys/compiler.h>

#ifdef __GNUC__

/*
 * Prototypes
 */
/*----------------------------------------------------------------------------
 * 2 words in registers
 *--------------------------------------------------------------------------*/
L4_INLINE int
l4_ipc_call(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_word0,
            l4_umword_t snd_word1,
            void *rcv_msg,
            l4_umword_t *rcv_word0,
            l4_umword_t *rcv_word1,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

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
            l4_msgtag_t *rtag);

L4_INLINE int
l4_ipc_reply_and_wait(l4_threadid_t dest,
                      const void *snd_msg,
                      l4_umword_t snd_word0,
                      l4_umword_t snd_word1,
                      l4_threadid_t *src,
                      void *rcv_msg,
                      l4_umword_t *rcv_word0,
                      l4_umword_t *rcv_word1,
                      l4_timeout_t timeout,
                      l4_msgdope_t *result);

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
                      l4_msgtag_t *rtag);

L4_INLINE int
l4_ipc_send(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t snd_word0,
            l4_umword_t snd_word1,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

L4_INLINE int
l4_ipc_send_tag(l4_threadid_t dest,
            const void *snd_msg,
            l4_umword_t w0,
            l4_umword_t w1,
            l4_msgtag_t tag,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_word0,
            l4_umword_t *rcv_word1,
            l4_timeout_t timeout,
            l4_msgdope_t *result);

L4_INLINE int
l4_ipc_wait_tag(l4_threadid_t *src,
            void *rcv_msg,
            l4_umword_t *rcv_w0,
            l4_umword_t *rcv_w1,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
            l4_msgtag_t *tag);

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_word0,
               l4_umword_t *rcv_word1,
               l4_timeout_t timeout,
               l4_msgdope_t *result);

L4_INLINE int
l4_ipc_receive_tag(l4_threadid_t src,
               void *rcv_msg,
               l4_umword_t *rcv_w0,
               l4_umword_t *rcv_w1,
               l4_timeout_t timeout,
               l4_msgdope_t *result,
               l4_msgtag_t *tag);

L4_INLINE int
l4_ipc_sleep(l4_timeout_t timeout);

#endif //__GNUC__

L4_INLINE int
l4_ipc_fpage_received(l4_msgdope_t msgdope);

L4_INLINE int
l4_ipc_is_fpage_granted(l4_fpage_t fp);

L4_INLINE int
l4_ipc_is_fpage_writable(l4_fpage_t fp);

L4_INLINE int
l4_is_rcv_map_descr(const void *msg);

L4_INLINE int
l4_is_long_rcv_descr(const void *msg);

L4_INLINE int
l4_is_long_snd_descr(const void *msg);

L4_INLINE const l4_msg_t *
l4_get_snd_msg_from_descr(const void *msg);


L4_INLINE l4_msg_t *
l4_get_rcv_msg_from_descr(void *msg);


/*----------------------------------------------------------------------------
 * IMPLEMENTATION
 *--------------------------------------------------------------------------*/

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

L4_INLINE int
l4_is_long_rcv_descr(const void *msg)
{
  if(!l4_is_rcv_map_descr(msg))
    return (int)msg & ~0x03;
  else 
    return 0;
}

L4_INLINE int 
l4_is_rcv_map_descr(const void *msg)
{
  return (int)msg & 0x2;
}

L4_INLINE int
l4_is_long_snd_descr(const void *msg)
{
  return (int)msg & ~0x03;
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


#include <l4/sys/ipc_api.h>


#endif

