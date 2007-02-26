#ifndef __L4_IPCMON_H
#define __L4_IPCMON_H

#include <l4/sys/types.h>

int l4ipcmon_allow(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest);
int l4ipcmon_allow_named(l4_threadid_t monitor, l4_taskid_t src, char *name);

int l4ipcmon_deny(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest);
int l4ipcmon_deny_named(l4_threadid_t monitor, l4_taskid_t src, char *name);

int l4ipcmon_query(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest);

#endif
