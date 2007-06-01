#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include <stdio.h>

int
main(int argc, char **argv)
{
  l4_threadid_t th;
  l4_msgdope_t result;
  l4_umword_t d1, d2;

  printf("Hello World\n");

  th = l4_myself();

  for (;;)
    {
      printf("hello: My thread-id is %x.%x\n", th.id.task, th.id.lthread);

      /* wait .5 sec */
      l4_ipc_receive(L4_NIL_ID, 0, &d1, &d2, l4_ipc_timeout(0,0,976,9), &result);

    }
}

