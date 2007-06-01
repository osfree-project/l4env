#include <l4/sys/ipc.h>

/*
 * generate inline functions to allow a look at the generated assembly code
 */

void ipc_test(void)
{
  l4_threadid_t dest, src;
  l4_timeout_t timeout;
  l4_umword_t snd_dword0, snd_dword1, rcv_dword0, rcv_dword1;
  int snd_msg, rcv_msg;
  l4_msgdope_t result;
  int ret;

  snd_dword0 = snd_dword1 = rcv_dword0 = rcv_dword1 = 1;
  result = L4_IPC_DOPE(1,1);
  timeout = l4_ipc_timeout(1, 1, 1, 1);
  timeout = L4_IPC_NEVER;

  asm(".ascii l4_ipc_call\n");
  ret = l4_ipc_call(dest,
		    &snd_msg,  snd_dword0,  snd_dword1,
		    &rcv_msg,  &rcv_dword0,  &rcv_dword1,
		    timeout,  &result);

  asm(".ascii l4_ipc_reply_and_wait\n");
  ret = l4_ipc_reply_and_wait(dest,
			      &snd_msg,
			      snd_dword0,  snd_dword1,
			      &src,
			      &rcv_msg,  &rcv_dword0,  &rcv_dword1,
			      timeout,  &result);

  asm(".ascii l4_ipc_send\n");
  ret = l4_ipc_send(dest,
		    &snd_msg,  snd_dword0,  snd_dword1,
		    timeout,  &result);
  asm(".ascii l4_ipc_wait\n");
  ret = l4_ipc_wait(&src,
		    &rcv_msg,  &rcv_dword0,  &rcv_dword1,
		    timeout,  &result);
  asm(".ascii l4_ipc_receive\n");
  ret = l4_ipc_receive(src,
		       &rcv_msg,  &rcv_dword0,  &rcv_dword1,
		       timeout,  &result);
}

