#include <stdio.h>

#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#include "global.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "helper_inc.h"

