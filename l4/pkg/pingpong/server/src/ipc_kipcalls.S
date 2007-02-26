
#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER
#define KIP_SYSCALLS
#define L4V2_IPC_SYSENTER
#define L4X0_IPC_SYSENTER
#undef PREFIX
#define PREFIX(a) kipcalls_ ## a
#include "ipc_inc.S"

