
#include <stdio.h>
#include <stdlib.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/semaphore/semaphore.h>

#include "direct.h"

static int key;
static l4semaphore_t key_sem = L4SEMAPHORE_INIT(0);

static void
key_thread(void *data)
{
  l4_threadid_t virq_id;
  l4_msgdope_t result;
  l4_umword_t d1, d2;
  int error;

  l4_make_taskid_from_irq(17, &virq_id);  
  error = l4_ipc_receive(virq_id, L4_IPC_SHORT_MSG, &d1, &d2,
			 L4_IPC_RECV_TIMEOUT_0, &result);
  if (error != L4_IPC_RETIMEOUT)
    {
      printf("Could not allocate virq, error=%02x\n", error);
      l4thread_exit();
    }

  if ((error = l4thread_started(0)))
    printf("Warning: sending startup notification failed, error=%d\n", error);

  for (;;)
    {
      error = l4_ipc_receive(virq_id, L4_IPC_SHORT_MSG, &d1, &d2,
				      L4_IPC_NEVER, &result);
      key = l4kd_inchar();
      l4semaphore_up(&key_sem);
    }
}

int
getchar(void)
{
  l4semaphore_down(&key_sem);
  return key;
}

void
key_init(void)
{
  if (l4thread_create(key_thread, 0, L4THREAD_CREATE_SYNC) < 0)
    {
      printf("Cannot create key thread\n");
      exit(-1);
    }
}
