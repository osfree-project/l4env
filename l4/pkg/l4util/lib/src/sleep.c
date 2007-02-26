/*
 * $Id$
 */

/*****************************************************************************
 * libl4util/src/sleep.c                                                     *
 * suspend thread                                                            *
 *****************************************************************************/

/* L4 includes */
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

void l4_sleep(int ms)
{
  int error,to_e,to_m;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4_timeout_t to;

  if (ms != -1)
    {
      /* calculate timeout */
      micros2l4to(ms * 1000,&to_e,&to_m);
      to = L4_IPC_TIMEOUT(0,0,to_m,to_e,0,0);
    }
  else
    to = L4_IPC_NEVER;

  error = l4_i386_ipc_receive(L4_NIL_ID,L4_IPC_SHORT_MSG,
                              &dummy,&dummy,to,&result);
 
  if (error != L4_IPC_RETIMEOUT)
    {
      outstring("(l4_sleep): IPC error\n\r");
    }
}


void l4_usleep(int us)
{
  int error,to_e,to_m;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4_timeout_t to;

  if (us != -1)
    {
      /* calculate timeout */
      micros2l4to(us,&to_e,&to_m);
      to = L4_IPC_TIMEOUT(0,0,to_m,to_e,0,0);
    }
  else
    to = L4_IPC_NEVER;

  error = l4_i386_ipc_receive(L4_NIL_ID,L4_IPC_SHORT_MSG,
                              &dummy,&dummy,to,&result);
 
  if (error != L4_IPC_RETIMEOUT)
    {
      outstring("(l4_usleep): IPC error\n\r");
    }
}


