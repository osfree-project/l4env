/*
 * $Id$
 */

/*****************************************************************************
 * libl4util/src/sleep.c                                                     *
 * suspend thread                                                            *
 *****************************************************************************/

#include <stdio.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

void l4_sleep(int ms)
{
  int error;
  l4_timeout_t to;

  if (ms != -1)
    {
      /* calculate timeout */
      to = l4_timeout(L4_IPC_TIMEOUT_NEVER,l4util_micros2l4to(ms * 1000));
    }
  else
    to = L4_IPC_NEVER;

  error = l4_ipc_sleep(to);

  if (error != L4_IPC_RETIMEOUT)
    printf("l4_sleep(): IPC error %02x\n", error);
}


void l4_usleep(int us)
{
  int error;
  l4_timeout_t to;

  /* calculate timeout */
  to = l4_timeout(L4_IPC_TIMEOUT_NEVER, l4util_micros2l4to(us));

  error = l4_ipc_sleep(to);
 
  if (error != L4_IPC_RETIMEOUT)
    printf("l4_usleep(): IPC error %02x\n", error);
}

