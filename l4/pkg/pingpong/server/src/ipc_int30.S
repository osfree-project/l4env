
#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER
#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "ipc_inc.S"

