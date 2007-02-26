#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>

#include "local.h"

int sigprocmask( int how, const sigset_t *set, sigset_t *oldset )
{
    int i;
    int me = l4thread_myself();
    sigset_t *currentset;
    
    l4semaphore_down( &l4signal_masktable_sem );
    
    currentset = &(l4signal_sigmask_table[me].blocked_mask);
    // store old set in oldset
    if (oldset)
        *oldset = *currentset;
    
    // we are done if the user just wanted to get
    // the current sigset
    if (!set)
        return 0;
    
    switch( how )
    {
        // add signals of set to currentset
        case SIG_BLOCK:
            for (i=0; i<=NSIG; i++)            
            {
                if (sigismember(set,i))
                    sigaddset(currentset,i);
            }
            break;
        // remove signals of set from currentset
        case SIG_UNBLOCK:
            for (i=0; i<=NSIG; i++)
            {
                if (sigismember(set, i))
                    sigdelset(currentset,i);
            }
            break;
        // set is the new block mask
        case SIG_SETMASK:
            l4signal_sigmask_table[me].blocked_mask = *set;
            break;
        default: break;
    }

    l4semaphore_up( &l4signal_masktable_sem );

    return 0;
}

int sigpending( sigset_t *set )
{
    int me = l4thread_myself();
    l4semaphore_down( &l4signal_masktable_sem );
    *set = l4signal_sigmask_table[me].pending_mask;
    l4semaphore_up( &l4signal_masktable_sem );
    return 0;
}
