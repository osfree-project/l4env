
#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER
#define L4V2_IPC_SYSENTER
#define L4X0_IPC_SYSENTER
#undef PREFIX
#define PREFIX(a) sysenter_ ## a
#include "ipc_inc.S"

