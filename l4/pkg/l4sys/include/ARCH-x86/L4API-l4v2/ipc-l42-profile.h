/* 
 * $Id$
 */

#ifndef __L4_IPC_L42_PROFILE_H__ 
#define __L4_IPC_L42_PROFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int
l4_i386_ipc_call_static(l4_threadid_t dest, 
			const void *snd_msg, 
			l4_umword_t snd_dword0, 
			l4_umword_t snd_dword1, 
			void *rcv_msg, 
			l4_umword_t *rcv_dword0, 
			l4_umword_t *rcv_dword1, 
			l4_timeout_t timeout, 
			l4_msgdope_t *result);
extern int
l4_i386_ipc_reply_and_wait_static(l4_threadid_t dest, 
				  const void *snd_msg, 
				  l4_umword_t snd_dword0, 
				  l4_umword_t snd_dword1, 
				  l4_threadid_t *src, 
				  void *rcv_msg, 
				  l4_umword_t *rcv_dword0, 
				  l4_umword_t *rcv_dword1,
				  l4_timeout_t timeout, 
				  l4_msgdope_t *result);
extern int
l4_i386_ipc_send_static(l4_threadid_t dest, 
			const void *snd_msg, 
			l4_umword_t snd_dword0, 
			l4_umword_t snd_dword1,
			l4_timeout_t timeout, 
			l4_msgdope_t *result);
extern int
l4_i386_ipc_wait_static(l4_threadid_t *src,
			void *rcv_msg, 
			l4_umword_t *rcv_dword0, 
			l4_umword_t *rcv_dword1, 
			l4_timeout_t timeout, 
			l4_msgdope_t *result);
extern int
l4_i386_ipc_receive_static(l4_threadid_t src,
			   void *rcv_msg, 
			   l4_umword_t *rcv_dword0, 
			   l4_umword_t *rcv_dword1, 
			   l4_timeout_t timeout, 
			   l4_msgdope_t *result);

#ifdef __cplusplus
}
#endif

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
  return l4_i386_ipc_call_static(dest, snd_msg, snd_dword0, snd_dword1,
				 rcv_msg, rcv_dword0, rcv_dword1, 
				 timeout, result);
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
  return l4_i386_ipc_reply_and_wait_static(dest, snd_msg, snd_dword0,
					   snd_dword1, src, rcv_msg, 
					   rcv_dword0, rcv_dword1, 
					   timeout, result);
}

L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, 
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  return l4_i386_ipc_send_static(dest, snd_msg, snd_dword0, snd_dword1,
				 timeout, result);
}

L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result)
{
  return l4_i386_ipc_wait_static(src, rcv_msg, rcv_dword0, rcv_dword1,
				 timeout, result);
}

L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, 
		    l4_umword_t *rcv_dword0, 
		    l4_umword_t *rcv_dword1, 
		    l4_timeout_t timeout, 
		    l4_msgdope_t *result)
{
  return l4_i386_ipc_receive_static(src, rcv_msg, rcv_dword0, rcv_dword1,
				    timeout, result);
}

#endif

