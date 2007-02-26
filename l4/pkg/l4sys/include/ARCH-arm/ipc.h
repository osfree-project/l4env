/* 
 * $Id$
 */

#ifndef L4_IPC_H 
#define L4_IPC_H

/*
 * L4 ipc
 */

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

/*
 * IPC parameters
 */

/* 
 * Defines used for Parameters 
 */

#define L4_IPC_SHORT_MSG 	0

/*
 * Defines used to build Parameters
 */

#define L4_IPC_STRING_SHIFT 8
#define L4_IPC_DWORD_SHIFT 13
#define L4_IPC_SHORT_FPAGE ((void *)2)
#define L4_IPC_OPEN_IPC 1

#define L4_IPC_DOPE(words, strings) \
( (l4_msgdope_t) {md: {0, 0, 0, 0, 0, 0, strings, words }})


#define L4_IPC_TIMEOUT(snd_man, snd_exp, rcv_man, rcv_exp, snd_pflt, rcv_pflt)\
     ( (l4_timeout_t) \
       {to: { rcv_exp, snd_exp, rcv_pflt, snd_pflt, snd_man, rcv_man } } )

#define L4_IPC_NEVER			((l4_timeout_t) {timeout: 0})
#define L4_IPC_NEVER_INITIALIZER	{timeout: 0}
#define L4_IPC_MAPMSG(address, size)  \
     ((void *)(l4_umword_t)( ((address) & L4_PAGEMASK) | ((size) << 2) \
			 | (unsigned long)L4_IPC_SHORT_FPAGE)) 


/* 
 * Some macros to make result checking easier
 */

#define L4_IPC_ERROR_MASK 	0xF0
#define L4_IPC_DECEIT_MASK	0x01
#define L4_IPC_FPAGE_MASK	0x02
#define L4_IPC_REDIRECT_MASK	0x04
#define L4_IPC_SRC_MASK		0x08
#define L4_IPC_SND_ERR_MASK	0x10

#define L4_IPC_IS_ERROR(x)		(((x).msgdope) & L4_IPC_ERROR_MASK)
#define L4_IPC_MSG_DECEITED(x) 		(((x).msgdope) & L4_IPC_DECEIT_MASK)
#define L4_IPC_MSG_REDIRECTED(x)	(((x).msgdope) & L4_IPC_REDIRECT_MASK)
#define L4_IPC_SRC_INSIDE(x)		(((x).msgdope) & L4_IPC_SRC_MASK)
#define L4_IPC_SND_ERROR(x)		(((x).msgdope) & L4_IPC_SND_ERR_MASK)
#define L4_IPC_MSG_TRANSFER_STARTED \
				((((x).msgdope) & L4_IPC_ERROR_MASK) < 5)



/*
 * IPC results
 */

#define L4_IPC_ERROR(x)			(((x).msgdope) & L4_IPC_ERROR_MASK)
#define L4_IPC_ENOT_EXISTENT		0x10
#define L4_IPC_RETIMEOUT		0x20
#define L4_IPC_SETIMEOUT		0x30
#define L4_IPC_RECANCELED		0x40
#define L4_IPC_SECANCELED		0x50
#define L4_IPC_REMAPFAILED		0x60
#define L4_IPC_SEMAPFAILED		0x70
#define L4_IPC_RESNDPFTO		0x80
#define L4_IPC_SESNDPFTO		0x90
#define L4_IPC_RERCVPFTO		0xA0
#define L4_IPC_SERCVPFTO		0xB0
#define L4_IPC_REABORTED		0xC0
#define L4_IPC_SEABORTED		0xD0
#define L4_IPC_REMSGCUT			0xE0
#define L4_IPC_SEMSGCUT			0xF0

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
l4_ipc_send(l4_threadid_t dest, 
            const void *snd_msg,
            l4_umword_t snd_word0, 
            l4_umword_t snd_word1, 
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
l4_ipc_receive(l4_threadid_t src,
               void *rcv_msg, 
               l4_umword_t *rcv_word0, 
               l4_umword_t *rcv_word1, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result);

/*----------------------------------------------------------------------------
 * 3 words in registers
 *--------------------------------------------------------------------------*/
L4_INLINE int
l4_ipc_call_w3(l4_threadid_t dest, 
               const void *snd_msg, 
               l4_umword_t snd_word0, 
               l4_umword_t snd_word1, 
               l4_umword_t snd_word2, 
               void *rcv_msg, 
               l4_umword_t *rcv_word0, 
               l4_umword_t *rcv_word1, 
               l4_umword_t *rcv_word2, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result);

L4_INLINE int
l4_ipc_reply_and_wait_w3(l4_threadid_t dest, 
                         const void *snd_msg, 
                         l4_umword_t snd_word0, 
                         l4_umword_t snd_word1, 
                         l4_umword_t snd_word2, 
                         l4_threadid_t *src,
                         void *rcv_msg, 
                         l4_umword_t *rcv_word0, 
                         l4_umword_t *rcv_word1, 
                         l4_umword_t *rcv_word2, 
                         l4_timeout_t timeout, 
                         l4_msgdope_t *result);

L4_INLINE int
l4_ipc_send_w3(l4_threadid_t dest, 
               const void *snd_msg,
               l4_umword_t snd_word0, 
               l4_umword_t snd_word1, 
               l4_umword_t snd_word2, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result);

L4_INLINE int
l4_ipc_wait_w3(l4_threadid_t *src,
               void *rcv_msg, 
               l4_umword_t *rcv_word0, 
               l4_umword_t *rcv_word1, 
               l4_umword_t *rcv_word2, 
               l4_timeout_t timeout, 
               l4_msgdope_t *result);

L4_INLINE int
l4_ipc_receive_w3(l4_threadid_t src,
                  void *rcv_msg, 
                  l4_umword_t *rcv_word0, 
                  l4_umword_t *rcv_word1, 
                  l4_umword_t *rcv_word2, 
                  l4_timeout_t timeout, 
                  l4_msgdope_t *result);


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

