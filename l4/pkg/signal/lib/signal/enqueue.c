#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/signal/signal-client.h>

#include "local.h"

extern int _DEBUG;

static void l4signal_enqueue_signal_nonrt(siginfo_t sig);
static void l4signal_enqueue_signal_rt(l4thread_t thread, siginfo_t sig);
static siginfo_t *l4signal_get_next_pending_nonrt(l4thread_t thread);
static siginfo_t *l4signal_get_next_pending_rt(l4thread_t thread);

/*  Enqueue a non-real-time signal. These signals may not stack each other.
 *  We therefore keep a global table of these signals and only enqueue a new
 *  signal, if there is no such signal already pending.
 */
static void l4signal_enqueue_signal_nonrt(siginfo_t sig)
{
    l4semaphore_down(&l4signal_signaltable_sem);
    // only add this signal if it was not received yet
    if (non_rt_signals[sig.si_signo-1] == NULL)
    {
        non_rt_signals[sig.si_signo-1] = &sig;
    }
    l4semaphore_up(&l4signal_signaltable_sem);
}

/* Enqueue a real-time signal. These signals may stack, which means another
   signal like this may be sent to the application even if the last one was
   not completely handled yet. We therefore keep a per-thread signal list
   which contains all signals to be handled by a thread.
 */
static void l4signal_enqueue_signal_rt(l4thread_t thread, siginfo_t sig)
{
    signal_data *head;
    signal_data *data = (signal_data *)malloc(sizeof(signal_data));
    data->signal = sig;
    data->next   = NULL;

    l4semaphore_down( &l4signal_masktable_sem );
    head = (l4signal_sigmask_table[thread].sigdata);

    if (head==NULL)
        l4signal_sigmask_table[thread].sigdata = data;
    else
    {
        while( head->next )
            head = head->next;
        head->next = data;
    }
    l4semaphore_up( &l4signal_masktable_sem );
}

/* Enqueue signal into the thread's pending signal list
 */
void l4signal_enqueue_signal(l4thread_t thread, siginfo_t sig)
{
    // only non-realtime signals are handled with a linked list
    if (sig.si_signo > 32)
        l4signal_enqueue_signal_rt(thread, sig);
    else
        l4signal_enqueue_signal_nonrt(sig);
}

/* Get the next pending non-real-time signal.
 */
static siginfo_t *l4signal_get_next_pending_nonrt(l4thread_t thread)
{
    int i=0;
    siginfo_t *ret = (siginfo_t *)malloc(sizeof(siginfo_t));

    l4semaphore_down(&l4signal_signaltable_sem);
    l4semaphore_down(&l4signal_masktable_sem);
    for (i=0; i<32; i++)
    {
        if (non_rt_signals[i] != NULL && !sigismember(&l4signal_sigmask_table[thread].blocked_mask, i+1))
        {
            LOGd(_DEBUG, "signal to handle: %d", non_rt_signals[i]->si_signo);
            memcpy(ret, non_rt_signals[i], sizeof(siginfo_t));
            free(non_rt_signals[i]);
            non_rt_signals[i] = NULL;
            l4semaphore_up(&l4signal_masktable_sem);
            l4semaphore_up(&l4signal_signaltable_sem);
            return ret;
        }
    }
    l4semaphore_up(&l4signal_masktable_sem);
    l4semaphore_up(&l4signal_signaltable_sem);

    free(ret);
    return NULL;
}

/* Get the next pending real-time signal.
 */
static siginfo_t *l4signal_get_next_pending_rt(l4thread_t thread)
{
    signal_data *head=NULL, *it=NULL, *prev=NULL;
    siginfo_t *ret = (siginfo_t *)malloc(sizeof(siginfo_t));

    l4semaphore_down(&l4signal_masktable_sem);
    head = (l4signal_sigmask_table[thread].sigdata);
    it = head;

    while(it && sigismember(&l4signal_sigmask_table[thread].blocked_mask, it->signal.si_signo))
    {
        prev = it;
        it = it->next;
    }
    l4semaphore_up(&l4signal_masktable_sem);

    if (it == NULL) // empty list OR no unblocked signal inside
    {
        LOGd(_DEBUG, "no more sigs.");
        free(ret);
        return NULL;
    }

    if (it == head) // it == head means, dequeue the first signal
    {
        l4semaphore_down(&l4signal_masktable_sem);
        l4signal_sigmask_table[thread].sigdata = it->next;
        l4semaphore_up(&l4signal_masktable_sem);
    }
    else
    {
        prev->next = it->next;
    }
    LOGd(_DEBUG, "it->sig = %p\n", &it->signal);

    // copying is slow, but in the next step we want to free
    // the list element and therefore need that data copied
    // elsewhere.
    // FIXME: The list element should contain only a pointer
    // to the siginfo_t. Then we could free without copying here.
    // Otherwise we needed to copy on enqueue of the element then.
    // What is better?
    memcpy(ret, &it->signal, sizeof(siginfo_t));
    free(it);
    return (ret);
}


/* Get next pending signal or NULL if none is left.
 */
siginfo_t *l4signal_get_next_pending(l4thread_t thread)
{
    siginfo_t *ret;

    ret = l4signal_get_next_pending_nonrt(thread);
    if (ret)
        return ret;

    // Now that we have not found pending non-RT signals, we check
    // the thread-local signal list for pending RT signals.
    ret = l4signal_get_next_pending_rt(thread);

    return ret;
}
