#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include "local.h"

sighandler_t signal( int signum, sighandler_t handler )
{
    sighandler_t old_handler;
    sigset_t new_set;

    // no one may set handlers for SIGKILL and SIGSTOP,
    // old handler is always SIG_DFL
    if (signum == SIGKILL || signum == SIGSTOP)
    {
        return SIG_DFL;
    }
    
    // fill in values
    sigfillset(&new_set);

    l4semaphore_down( &l4signal_handlertable_sem );

    old_handler                               = l4signal_sighandler_table[signum].sa_handler;
    l4signal_sighandler_table[signum].sa_handler     = handler;
    l4signal_sighandler_table[signum].sa_flags       = SA_ONESHOT;
    l4signal_sighandler_table[signum].sa_restorer    = NULL;
    l4signal_sighandler_table[signum].sa_mask        = new_set;
    
    l4semaphore_up( &l4signal_handlertable_sem );

    return old_handler;

}
