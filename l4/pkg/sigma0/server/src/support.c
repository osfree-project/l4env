#include <l4/sys/ipc.h>

void *__dso_handle = 0;

void _exit(int) __attribute__((noreturn));

void _exit(int x)
{ l4_ipc_sleep(L4_IPC_NEVER);  while (1) ; }
