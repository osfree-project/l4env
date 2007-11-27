#include <stdio.h>

#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER
#undef CONFIG_L4_CALL_SYSCALLS
#define L4V2_IPC_SYSENTER
#define L4X0_IPC_SYSENTER

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/bitops.h>

#include "global.h"
#include "ipc_buffer.h"
#include "worker.h"
#include "pingpong.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) sysenter_ ## a
#include "worker_inc_x86.h"

