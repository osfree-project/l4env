#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/signal/signal-client.h>

#include "local.h"

extern l4_threadid_t l4signal_signal_server_id;

int sigtimedwait(const sigset_t *mask, siginfo_t *info, const struct timespec *ts)
{
    LOG("unimplemented");
    return 0;
}
