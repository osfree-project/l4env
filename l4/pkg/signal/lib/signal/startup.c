#include <stdlib.h>

#include <dice/dice.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/crtx/ctor.h>

#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/signal/signal-client.h>
#include <l4/names/libnames.h>

#include "local.h"

//extern int _DEBUG;

l4_threadid_t l4signal_signal_server_id;

// signal thread
void l4signal_signal_thread(void *argp)
{
    int i=0;
    sigset_t blocking, empty_set;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc      = (dice_malloc_func)malloc;
    _dice_corba_env.free        = (dice_free_func)free;


//    LOGd(_DEBUG, "\033[33msignal thread started: "l4util_idfmt"\033[37m",
//            l4util_idstr(l4_myself()));

    // startup notification
    l4thread_started(0);

    l4signal_bazar.producer = L4SEMAPHORE_UNLOCKED;
    l4signal_bazar.consumer = L4SEMAPHORE_LOCKED;

    sigemptyset( &empty_set );

    // 1. init data
    l4signal_masktable_sem = L4SEMAPHORE_LOCKED;
    l4signal_handlertable_sem = L4SEMAPHORE_LOCKED;
    l4signal_signaltable_sem = L4SEMAPHORE_LOCKED;

    for (i=0; i<THREAD_MAX; i++)
    {
        // no pending signals at the beginning
        sigemptyset( &(l4signal_sigmask_table[i].pending_mask) );

        // make sure all threads have an empty blockmask
        // at the beginning
        sigemptyset( &(l4signal_sigmask_table[i].blocked_mask) );

//        if (THREAD_EXISTS(i))
//        {
//            l4signal_thread_prio[i] = l4thread_get_prio(i);
//        }
    }

    for (i=0; i<NSIG; i++)
    {
        // default signal handler for every signal
        l4signal_sighandler_table[i].sa_handler  = SIG_DFL;
        l4signal_sighandler_table[i].sa_mask     = empty_set;
        l4signal_sighandler_table[i].sa_flags    = 0;
        l4signal_sighandler_table[i].sa_restorer = NULL;
        // do not block any signals while in sighandler
        sigemptyset( &l4signal_sighandler_table[i].sa_mask );
        l4signal_sighandler_table[i].sa_flags    = 0;
    }

    // initialize non-RT-signal-table
    for (i=0; i<32; i++)
    {
        non_rt_signals[i] = NULL;
    }


    // 2. lookup signal server at names
    i = names_query_name( "sig_serv", &l4signal_signal_server_id);
    do {
        LOG("signal server not found at names. waiting.");
        l4_sleep(1000);
        i = names_query_name( "sig_serv", &l4signal_signal_server_id);
    }
    while (i==0);
    //LOGd(_DEBUG, "query names for signal_server: %d", i);

    // 3. register at signal_server
    if (i)
    {
        //LOGd(_DEBUG, "registering at signal_server");
        i = signal_signal_register_handler_call( &l4signal_signal_server_id,
                &_dice_corba_env );
        //LOGd(_DEBUG, "registered at sig server: %d", i);
    }

    l4semaphore_up( &l4signal_masktable_sem );
    l4semaphore_up( &l4signal_handlertable_sem );
    l4semaphore_up( &l4signal_signaltable_sem );

    // create a filled sigset_t
    sigfillset( &blocking );
    // set all signals to be blocked for this thread
    sigprocmask( SIG_SETMASK, &blocking, NULL );

    // 4. receive and dispatch signals
    while(1)
    {
        siginfo_t sig;
        l4_threadid_t thread;
        //LOGd(_DEBUG, "ready to receive signals");
        sig = signal_signal_receive_signal_call( &l4signal_signal_server_id,
            &thread, &_dice_corba_env );

//        LOGd(_DEBUG, "signal received: %d, code: %d", sig.si_signo, sig.si_code);
        //LOGd(_DEBUG, "recipient: "l4util_idfmt, l4util_idstr(thread));
        l4signal_dispatch_signal( sig, thread );
    }
}

// constructor to launch the signal thread
void l4signal_init_signals(void)
{
    l4thread_t thread;
    LOG("starting signal thread");

    thread = l4thread_create( l4signal_signal_thread, (void *)NULL, L4THREAD_CREATE_SYNC );
}
L4C_CTOR(l4signal_init_signals, 3100);
