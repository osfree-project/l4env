#ifndef HELPER_H
#define HELPER_H

extern void call(l4_threadid_t tid);
extern void send(l4_threadid_t tid);
extern void recv(l4_threadid_t tid);
extern void recv_ping_timeout(l4_timeout_t timeout);

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "helper_if.h"

#undef PREFIX
#define PREFIX(a) sysenter_ ## a
#include "helper_if.h"

#undef PREFIX
#define PREFIX(a) kipcalls_ ## a
#include "helper_if.h"

#endif

