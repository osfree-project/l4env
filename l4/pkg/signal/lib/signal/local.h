#ifndef l4signal_L4_SIGNAL_H
#define l4signal_L4_SIGNAL_H

#include <l4/signal/l4signal.h>
#include <signal.h>

#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/log/l4log.h>

#include "exceptions.h"

#define THREAD_EXISTS(x) !l4_thread_equal( l4thread_l4_id((x)), L4_INVALID_ID )
#define THREAD_MAX 128

// startup stuff
void l4signal_signal_thread(void *argp);
void l4signal_init_signals(void);

// Non-RT signals (1-32) may only be delivered one at a time.
// We therefore maintain an array of siginfo_t pointers for these
// signals.
extern siginfo_t *non_rt_signals[32];

// For RT signals (>32) we need to manage a list of pointers, because
// these signals may occur multiple times.
typedef struct signal_data
{
    struct signal_data *next;
    siginfo_t signal;
} signal_data;

// we need a table holding the current signal mask and some
// other data for every thread we know. This struct defines
// one entry in it.
typedef struct thread_sig_mask
{
    sigset_t    pending_mask;          // sigset of pending signals
    sigset_t    blocked_mask;          // sigset of blocked signals
    signal_data *sigdata;              // list of pending signals
    l4_umword_t esp;
    l4_umword_t eip;
} thread_sig_mask_t;

// we have a producer-consumer-problem on entering a signal
// handler: the signal thread choses a handler thread and
// forces it into the signal function. The handling thread
// afterwards needs to know to where it has to return and therefore
// must read its old eip, esp and flags from somewhere.
// Unfortunately we get this info just after we set the handler
// thread into the new function.
typedef struct
{
    l4semaphore_t producer;
    l4semaphore_t consumer;
    l4_umword_t   eip;
    l4_umword_t   esp;
    l4_umword_t   flags;
//    siginfo_t     signal;
} l4signal_bazar_t;

extern l4signal_bazar_t l4signal_bazar;

// the signal mask table -> there is a pending and a blocked mask
// for every thread of the task
extern thread_sig_mask_t l4signal_sigmask_table[THREAD_MAX];

// the signal handler table -> there is only one table
// per task
extern struct sigaction l4signal_sighandler_table[NSIG];

// the sigmask table must be secured by a semaphore because the handler
// thread and all other threads may access it
extern l4semaphore_t    l4signal_masktable_sem;

// the same goes for the handler table
extern l4semaphore_t    l4signal_handlertable_sem;

// a semaphore for the non-rt signal table
extern l4semaphore_t    l4signal_signaltable_sem;

// globally store signal server id as it is used
// by kill() from an arbitary thread
// l4_threadid_t l4signal_signal_server_id;

void l4signal_dispatch_signal( siginfo_t signal, l4_threadid_t thread );
void l4signal_signal_handling(void);
void l4signal_default_handling( siginfo_t *signal );
void l4signal_stop_app(void);
void l4signal_continue_app(void);
void l4signal_sleep(void);
void l4signal_shutdown_app(void);
l4thread_t l4signal_find_thread_for_signal( int sig );
l4thread_t l4signal_find_thread_alive(void);
void l4signal_enqueue_signal( l4thread_t thread, siginfo_t sig );
siginfo_t *l4signal_get_next_pending(l4thread_t thread);

#endif
