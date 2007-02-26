#ifndef WORKER_H
#define WORKER_H

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "worker_if.h"

#undef PREFIX
#define PREFIX(a) sysenter_ ## a
#include "worker_if.h"

extern void __attribute__((noreturn)) ping_exception_thread(void);

#endif

