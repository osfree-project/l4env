#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sigma0/kip.h>

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
  l4_threadid_t th;
  l4_msgdope_t result;
  l4_umword_t d1, d2;
  l4_kernel_clock_t next_timeout;
  l4_kernel_info_t *kip = l4sigma0_kip_map(L4_INVALID_ID);
  l4_timeout_t to = L4_IPC_BOTH_TIMEOUT_0;

  printf("Hello World\n");

  th = l4_myself();

  for (;;)
    {
      next_timeout = kip->clock;
      printf("hello: My thread-id is %x.%x %lld\n", th.id.task, th.id.lthread, kip->clock);

      next_timeout += 1500000;
      l4_rcv_timeout(l4_timeout_abs(next_timeout, L4_TIMEOUT_ABS_V2_s), &to);
      l4_ipc_receive(th, L4_IPC_SHORT_MSG, &d1, &d2, to, &result);
    }
}
