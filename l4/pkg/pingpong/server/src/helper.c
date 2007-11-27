
#include <l4/sys/types.h>

#include "helper.h"
#include "global.h"

void
call(l4_threadid_t tid)
{
#ifdef BENCH_GENERIC
  generic_call(tid);
#else
#ifdef BENCH_x86
  if (callmode == 2)
    kipcalls_call(tid);
  else if (callmode == 1)
    sysenter_call(tid);
  else
    int30_call(tid);
#else
#error .
#endif
#endif
}

void
send(l4_threadid_t tid)
{
#ifdef BENCH_GENERIC
  generic_send(tid);
#else
#ifdef BENCH_x86
  if (callmode == 2)
    kipcalls_send(tid);
  else if (callmode == 1)
    sysenter_send(tid);
  else
    int30_send(tid);
#else
#error .
#endif
#endif
}

void
recv(l4_threadid_t tid)
{
#ifdef BENCH_GENERIC
  generic_recv(tid);
#else
#ifdef BENCH_x86
  if (callmode == 2)
    kipcalls_recv(tid);
  else if (callmode)
    sysenter_recv(tid);
  else
    int30_recv(tid);
#else
#error .
#endif
#endif
}

void
recv_ping_timeout(l4_timeout_t timeout)
{
#ifdef BENCH_GENERIC
  generic_recv_ping_timeout(timeout);
#else
#ifdef BENCH_x86
  if (callmode == 2)
    kipcalls_recv_ping_timeout(timeout);
  else if (callmode == 1)
    sysenter_recv_ping_timeout(timeout);
  else
    int30_recv_ping_timeout(timeout);
#else
#error .
#endif
#endif
}

