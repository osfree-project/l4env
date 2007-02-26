
#include <l4/sys/types.h>

#include "helper.h"
#include "global.h"

void
call(l4_threadid_t tid)
{
  if (callmode == 2)
    kipcalls_call(tid);
  else if (callmode == 1)
    sysenter_call(tid);
  else
    int30_call(tid);
}

void
send(l4_threadid_t tid)
{
  if (callmode == 2)
    kipcalls_send(tid);
  else if (callmode == 1)
    sysenter_send(tid);
  else
    int30_send(tid);
}

void
recv(l4_threadid_t tid)
{
  if (callmode == 2)
    kipcalls_recv(tid);
  else if (callmode)
    sysenter_recv(tid);
  else
    int30_recv(tid);
}

void
recv_ping_timeout(l4_timeout_t timeout)
{
  if (callmode == 2)
    kipcalls_recv_ping_timeout(timeout);
  else if (callmode == 1)
    sysenter_recv_ping_timeout(timeout);
  else
    int30_recv_ping_timeout(timeout);
}

