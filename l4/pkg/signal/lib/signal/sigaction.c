#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>

#include "local.h"

int sigaction( int signum, const struct sigaction *act, struct sigaction *oldact )
{
    // do not set up signal handlers for SIGKILL and SIGSTOP
    if (signum == SIGKILL)
        return -EINVAL;
    if (signum == SIGSTOP)
        return -EINVAL;

    // save old action if valid pointer given
    if (oldact)   
    {
        l4semaphore_down(&l4signal_handlertable_sem);
        *oldact = l4signal_sighandler_table[signum];
        l4semaphore_up(&l4signal_handlertable_sem);
    }

    // set new sigaction if act is non-null
    if (act)
    {
        l4semaphore_down(&l4signal_handlertable_sem);
        l4signal_sighandler_table[signum] = *act;
        l4semaphore_up(&l4signal_handlertable_sem);
    }

    return 0;
}
