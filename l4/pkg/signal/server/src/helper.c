#include <l4/log/l4log.h>
#include <signal.h>

#include "local.h"

void dump_task_list(void)
{
    task_signal_struct_t *t = task_list_head;
    while (t)
    {
        LOG("(task %d, thread=%d.%d)->", t->taskno, t->signal_handler.id.task,
                t->signal_handler.id.lthread );
        t = t->next;
    }
    LOG("NULL");
}

void dump_signal_list( signal_struct_t *first )
{
    signal_struct_t *sig = first;
    while (sig)
    {
        siginfo_t signal = sig->signal;
        LOG("(signal %d, code %d) ->", signal.si_signo, signal.si_code );
        sig = sig->next;
    }
    LOG("NULL");
}
