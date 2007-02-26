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
extern l4_threadid_t l4signal_signal_server_id;

/* Standard function into which a thread is forced when it
 * received a signal. Does all signal handling necessary and
 * then returns to the point where it came from.
 */
void l4signal_signal_handling(void)
{
    // signal data
    siginfo_t *the_signal;
    // need that to thread_ex_regs() back
    int eip, esp, stored_stackpointer;
    l4_threadid_t pager;
    l4_threadid_t preempter;
    int dummy;
    // the block mask is different when inside a
    // signal handler
    sigset_t old_blocked, new_blocked;
    // always good to know yourself... :)
    l4thread_t me;
    struct sigaction action;

    (void)dummy;
    /* save the current register state to the stack
     * so that we can restore it later
     */
    asm volatile(
            "pusha          \n"
            "movl %%esp, %0 \n"
            : "=m"(stored_stackpointer)
            );

    the_signal = NULL;
    pager = preempter = L4_INVALID_ID;
    me = l4thread_id(l4_myself());

    // consumer part
    l4semaphore_down(&l4signal_bazar.consumer);
    eip        = l4signal_bazar.eip;
    esp        = l4signal_bazar.esp;
    l4semaphore_up(&l4signal_bazar.producer);

    // while there are pending signals
    while((the_signal = l4signal_get_next_pending(me)) != NULL)
    {
        // read sigaction
        l4semaphore_down(&l4signal_handlertable_sem);
        action = l4signal_sighandler_table[the_signal->si_signo];
        l4semaphore_up(&l4signal_handlertable_sem);

        // find out which signals to block during sighandler
        new_blocked = action.sa_mask;
        // store old blockmask, set new one
        l4semaphore_down(&l4signal_masktable_sem);
        old_blocked = l4signal_sigmask_table[me].blocked_mask;
        l4signal_sigmask_table[me].blocked_mask = new_blocked;
        l4semaphore_up(&l4signal_masktable_sem);

        // default handling
        if (action.sa_handler == SIG_DFL)
        {
            //LOGd(_DEBUG, "default handling");
            l4signal_default_handling(the_signal);
        }
        // call sigaction function having 3 parameters
        // FIXME: ucontext is not set at the moment
        else if (action.sa_flags & SA_SIGINFO )
        {
            //LOGd(_DEBUG, "signal function with 3 args.");
            (*action.sa_sigaction)(the_signal->si_signo, the_signal, NULL);
        }
        // call standard signal( int sig ) function
        else
        {
            //LOGd(_DEBUG, "signal function with 1 argument.");
            (*action.sa_handler)(the_signal->si_signo);
        }

        // if this was a oneshot signal, reset to default handler
        if (( action.sa_flags & SA_ONESHOT) ||
            ( action.sa_flags & SA_RESETHAND)  )
        {
            signal( the_signal->si_signo, SIG_DFL );
        }

        // restore old block mask. No race condition here, because the
        // mask will only be set and reset by the thread itself. The
        // thread is actually executing this code and will not permanently
        // change it's block mask. Therefore it is safe to restore the
        // old mask here.
        l4semaphore_down(&l4signal_masktable_sem);
        l4signal_sigmask_table[me].blocked_mask = old_blocked;
        // remove the signal from the pending list
        sigdelset( &l4signal_sigmask_table[me].pending_mask, the_signal->si_signo);
        l4semaphore_up(&l4signal_masktable_sem);

        // free signal
        free(the_signal);
    }

    /* restore registers, then jump back */
    asm volatile(
            "movl %0, %%esp     \n"     // load stored stackp
            "popa               \n"     // pop registers
            "movl %1, %%esp     \n"     // restore stackpointer
            "pushl %2           \n"     // restore instruction pointer
            "ret               \n"      // return
            :
            : "m"(stored_stackpointer),
              "m"(esp),
              "m"(eip)
            );

/*    //LOGd(_DEBUG,"no more signals...");
    // warp back to where we came from
    l4_thread_ex_regs( l4_myself(), eip, esp, &preempter, &pager,
        &dummy, &dummy, &dummy );
*/
}

/* Default signal handling. All signals not caught by the user
 * end up here.
 */
void l4signal_default_handling( siginfo_t *signal )
{
    //LOG("handling signal: %d", signal->si_signo);
    switch( signal->si_signo )
    {
        case SIGKILL:
        case SIGHUP:        // terminate
        case SIGINT:
        case SIGPIPE:
        case SIGALRM:
        case SIGTERM:
        case SIGUSR1:
        case SIGUSR2:
        case SIGPROF:
        case SIGVTALRM:
        case SIGIO:
#ifndef USE_UCLIBC
	// BTW: dietlibc defines SIGLOST to 30, Linux 2.6.12 to 29!
        case SIGLOST:
#endif
            LOG("Killed by Signal %d.", signal->si_signo);
            l4signal_shutdown_app();
            break;
        case SIGQUIT:       // dump core and then terminate
        case SIGILL:
        case SIGABRT:
        case SIGFPE:
        case SIGSEGV:
        case SIGBUS:
        case SIGSYS:
        case SIGTRAP:
        case SIGXCPU:
        case SIGXFSZ:
            enter_kdebug("Forced into kdebug by signal.");
            // after kdebug, the app shall be shut down
            l4signal_shutdown_app();
            break;
        case SIGSTOP:       // stop process
        case SIGTTIN:
        case SIGTTOU:
            l4signal_stop_app();
            break;
        case SIGCONT:
            // The application has already been continued by
            // dispatch_signal. Thus we silently ignore SIGCONT
            // here.
            break;
        case SIGCHLD:       // ignore
        case SIGWINCH:
        default:
            LOG("ignoring signal: %d", signal->si_signo);
            break;
    }
}

// put all application threads asleep
void l4signal_stop_app()
{
    int i;
    l4_umword_t dummy;
    // me, myself and I
    l4thread_t me = l4thread_id(l4_myself());
    static l4_threadid_t inval = L4_INVALID_ID;

    /* the original idea here was to schedule all working
     * threads with priority 0. Actually this won't work, because
     * someone might send an IPC to the threads and then the
     * time donation mechanism of the kernel would bring them
     * to some different prio.
     *
     * The better solution is, to thread_ex_regs them into
     * a function which sleeps forever.
     */
    LOG("application stopped by signal.");
    for (i=THREAD_MAX; i>1; i--)
    {
       if (THREAD_EXISTS(i) && i != me)
       {
           l4semaphore_down(&l4signal_masktable_sem);
           l4_thread_ex_regs(l4thread_l4_id(i), (l4_umword_t)l4signal_sleep,
                             (l4_umword_t)-1, &inval, &inval,
			     &dummy, &(l4signal_sigmask_table[i].eip),
			     &(l4signal_sigmask_table[i].esp));

//           l4signal_thread_prio[i] = l4thread_get_prio(i);
//           l4thread_set_prio(i, 0);
           l4semaphore_up(&l4signal_masktable_sem);
       }
    }
}

void l4signal_sleep(void)
{
    l4_msgdope_t res;
    // sleep forever
    l4_ipc_send(L4_INVALID_ID, 0, 0, 0, L4_IPC_NEVER, &res);
}

// wake all application threads
void l4signal_continue_app()
{
   int i;
   l4_umword_t dummy;
   l4thread_t me = l4thread_id(l4_myself());
   static l4_threadid_t inval = L4_INVALID_ID;

   // reset all priorities to old values
   for (i=THREAD_MAX; i>1; i--)
   {
       if (THREAD_EXISTS(i) && i != me)
       {
           l4semaphore_down(&l4signal_masktable_sem);
           l4_thread_ex_regs(l4thread_l4_id(i), l4signal_sigmask_table[i].eip,
                             l4signal_sigmask_table[i].esp, &inval, &inval, 
			     &dummy, &dummy, &dummy);
//           l4thread_set_prio(i, l4signal_thread_prio[i]);
           l4semaphore_up(&l4signal_masktable_sem);
       }
   }
   LOG("application continuing");
}

void l4signal_shutdown_app()
{
    int i;
    // me, myself and I
    l4thread_t me = l4thread_id(l4_myself());

    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc      = (dice_malloc_func)malloc;
    _dice_corba_env.free        = (dice_free_func)free;

    LOG("application killed by signal.");

    // shutdown all threads except me (as this would cause other
    // threads to remain)
    for (i=THREAD_MAX; i>=0; i--)
    {
        if (THREAD_EXISTS(i))
        {
            if (i!=me)
                l4thread_shutdown(i);
        }
    }

    // unregister at sigserver
    signal_signal_unregister_handler_call( &l4signal_signal_server_id,
            &_dice_corba_env );

    // bye bye
    l4thread_exit();
}
