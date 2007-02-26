#include "local.h"

// Non-RT signals (1-32) may only be delivered one at a time.
// We therefore maintain an array of siginfo_t pointers for these
// signals.
siginfo_t *non_rt_signals[32];

// the signal mask table -> there is a pending and a blocked mask
// for every thread of the task
thread_sig_mask_t l4signal_sigmask_table[THREAD_MAX];

// the signal handler table -> there is only one table
// per task
struct sigaction l4signal_sighandler_table[NSIG];

// the sigmask table must be secured by a semaphore because the handler
// thread and all other threads may access it
l4semaphore_t    l4signal_masktable_sem;

// the same goes for the handler table
l4semaphore_t    l4signal_handlertable_sem;

// a semaphore for the non-rt signal table
l4semaphore_t    l4signal_signaltable_sem;

l4signal_bazar_t l4signal_bazar;
