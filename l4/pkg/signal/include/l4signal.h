#ifndef __L4_SIGNAL_H
#define __L4_SIGNAL_H

#include <sys/types.h>
#include <signal.h>

#define THREADID_FROM_PID(tid,pid) ((tid).raw = (pid))
#define PID_FROM_THREADID(x) ((pid_t)(x.raw))

// init idt for a thread so that the thread is
// able to receive signals from cpu exceptions
void l4signal_init_idt(void);

// send a kill with a specific siginfo_t struct
// to pid
int l4signal_kill_long(pid_t, siginfo_t *);

#endif
