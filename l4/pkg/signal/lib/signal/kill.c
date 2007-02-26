#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/signal/signal-client.h>

#include "local.h"

extern l4_threadid_t l4signal_signal_server_id;

int kill( pid_t pid, int sig )
{
    siginfo_t signal;
    l4_threadid_t thread;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc      = (dice_malloc_func)malloc;
    _dice_corba_env.free        = (dice_free_func)free;

    THREADID_FROM_PID(thread, pid);

    switch (sig)
    {
        // fill in signal info here
        default: signal.si_signo = sig;
    }

    return signal_signal_kill_call( &l4signal_signal_server_id,
            &thread, &signal, &_dice_corba_env );
}

int raise( int sig )
{
    return kill( PID_FROM_THREADID(l4_myself()), sig );
}

int l4signal_kill_long(pid_t pid, siginfo_t *signal)
{
    l4_threadid_t thread;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc      = (dice_malloc_func)malloc;
    _dice_corba_env.free        = (dice_free_func)free;
    THREADID_FROM_PID(thread, pid);

    return signal_signal_kill_call( &l4signal_signal_server_id,
            &thread, signal, &_dice_corba_env);
}
