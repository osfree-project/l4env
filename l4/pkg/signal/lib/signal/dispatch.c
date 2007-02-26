#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/thread/thread.h>
#include <l4/signal/signal-client.h>

#include "local.h"

//extern int _DEBUG;

/* Find a thread not blocking this signal. */
l4thread_t l4signal_find_thread_for_signal( int sig )
{
    int i=2;

    l4semaphore_down( &l4signal_masktable_sem );

    // skip threads which block this signal and threads
    // which are not existing
    while (sigismember(&(l4signal_sigmask_table[i].blocked_mask), sig) ||
            !THREAD_EXISTS(i) )
        i++;

    l4semaphore_up( &l4signal_masktable_sem );

    // if we are out of threads, there is no thread ready
    // to accept this signal at the moment
    if (i>THREAD_MAX)
        return -1;

    return i;
}

/* Find the first running thread */
l4thread_t l4signal_find_thread_alive()
{
    int i=2;

    // skip all non-existent threads
    while (!THREAD_EXISTS(i) || (i==l4thread_id(l4_myself())))
        i++;

    return i;
}

/* Dispatch a signal. If the signal is not delivered to a specific
 * thread, determine an executor. Then force the executor into
 * a signal handling routine.
 */
void l4signal_dispatch_signal( siginfo_t signal, l4_threadid_t thread )
{
    l4thread_t executor = -1;
    l4_umword_t flags, old_eip, old_esp;
    l4_threadid_t pager, preempter;

    // shortcuts for SIGKILL and SIGSTOP as these must not
    // be caught or blocked
    if (signal.si_signo == SIGKILL)
    {
        l4signal_shutdown_app();
        return;
    }

    if (signal.si_signo == SIGSTOP)
    {
        l4signal_stop_app();
        return;
    }

    // another shortcut --> the signal thread must always handle
    // SIGCONT itself because all other threads might be sleeping after
    // a SIGSTOP and thus setting another thread to the sighandling function
    // might not work
    if (signal.si_signo == SIGCONT)
    {
        l4signal_continue_app();
        // do not return yet, because the user might have
        // installed a signal handler for SIGCONT and thus
        // we have to go through all the handling stuff below
    }

    // if the user chose to ignore this signal, we do not need to
    // go on any further
    l4semaphore_down(&l4signal_handlertable_sem);
    if (l4signal_sighandler_table[signal.si_signo].sa_handler == SIG_IGN)
    {
        l4semaphore_up(&l4signal_handlertable_sem);
        return;
    }
    l4semaphore_up(&l4signal_handlertable_sem);

    // recipient thread empty --> dispatch to first thread you find
    if (l4_thread_equal(thread, L4_INVALID_ID))
    {
        executor = l4signal_find_thread_for_signal( signal.si_signo );
        if (executor < 0)
        {
            // no thread is currently accepting this signal.
            // therefore we mark it pending for the first thread
            // alive
            //LOGd(_DEBUG,"no free executor, marking signal as pending");
            l4semaphore_down( &l4signal_masktable_sem );
            executor = l4signal_find_thread_alive();
            sigaddset(&(l4signal_sigmask_table[executor].pending_mask),
                        signal.si_signo);
            l4semaphore_up( &l4signal_masktable_sem );
            l4signal_enqueue_signal( executor, signal );
            return;
        }
    }
    else // try to dispatch to thread given
    {
        // determine thread id
        l4thread_t t = l4thread_id(thread);
        // check if thread exists
        if (THREAD_EXISTS(t))
        {
            // mark pending if thread is currently blocking the signal
            if (sigismember(&(l4signal_sigmask_table[t].blocked_mask), signal.si_signo))
            {
                l4semaphore_down( &l4signal_masktable_sem );
                sigaddset(&(l4signal_sigmask_table[t].pending_mask), signal.si_signo);
                l4semaphore_up( &l4signal_masktable_sem );
                l4signal_enqueue_signal( executor, signal );
                return;
            }
            else
                executor = t;

        }
        else
        {
            LOG("invalid recipient thread specified for signal %d", signal.si_signo);
            return;
        }
    }

    // Now that we have found an executor, enqueue the signal to its signal list
    l4signal_enqueue_signal( executor, signal );

    // invalidate pager and preempter, so that thread_ex_regs() does
    // set them to the real values
    preempter = L4_INVALID_ID;
    pager     = L4_INVALID_ID;

    // ready to handle the signal
    //LOGd(_DEBUG,"executor: "l4util_idfmt, l4util_idstr(l4thread_l4_id(executor)));

    // force executor into sighandling function
    //LOGd(_DEBUG, "forcing into sighandling function.");
    // producer part
    // down the semaphore BEFORE thread_ex_regs so that we hold it when the
    // executor wants to read data.
    l4semaphore_down(&l4signal_bazar.producer);
    l4_thread_ex_regs( l4thread_l4_id(executor), (l4_umword_t)l4signal_signal_handling,
        0xFFFFFFFF, &preempter, &pager, &flags,
        &old_eip, &old_esp);
    l4signal_bazar.eip    = old_eip;
    l4signal_bazar.esp    = old_esp;
    l4signal_bazar.flags  = flags;
//    bazar.signal = signal;
    l4semaphore_up(&l4signal_bazar.consumer);

}
