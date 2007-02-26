
#include <l4/sys/types.h>

#include "helper.h"
#include "global.h"

void
call(l4_threadid_t tid)
{
  if (sysenter)
    sysenter_call(tid);
  else
    int30_call(tid);
}

void
send(l4_threadid_t tid)
{
  if (sysenter)
    sysenter_send(tid);
  else
    int30_send(tid);
}

void
recv(l4_threadid_t tid)
{
  if (sysenter)
    sysenter_recv(tid);
  else
    int30_recv(tid);
}

void
recv_ping_timeout(l4_timeout_t timeout)
{
  if (sysenter)
    sysenter_recv_ping_timeout(timeout);
  else
    int30_recv_ping_timeout(timeout);
}

