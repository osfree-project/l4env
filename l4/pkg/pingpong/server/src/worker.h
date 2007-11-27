#ifndef WORKER_H
#define WORKER_H

#ifdef BENCH_GENERIC

#undef PREFIX
#define PREFIX(a) generic_ ## a
#include "worker_if.h"

#elif defined(BENCH_x86)

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "worker_if.h"

#undef PREFIX
#define PREFIX(a) sysenter_ ## a
#include "worker_if.h"

#undef PREFIX
#define PREFIX(a) kipcalls_ ## a
#include "worker_if.h"

#else
#error .
#endif

void __attribute__((noreturn)) ping_exception_intraAS_idt_thread(void);
void __attribute__((noreturn)) ping_exception_interAS_idt_thread(void);
void __attribute__((noreturn)) exception_reflection_pong_handler(void);
void __attribute__((noreturn)) ping_exception_IPC_thread(void);
void __attribute__((noreturn)) exception_IPC_pong_handler(void);

void __attribute__((noreturn)) ping_utcb_ipc_thread(void);
void __attribute__((noreturn)) pong_utcb_ipc_thread(void);

#endif
