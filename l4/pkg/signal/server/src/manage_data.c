#include <signal.h>
#include <stdlib.h>

#include <l4/log/l4log.h>
#include <l4/util/rdtsc.h>
#include "local.h"

extern int _DEBUG;

int enqueue_handler( l4_threadid_t *sigthread )
{
    int taskno = (*sigthread).id.task;
    
    task_signal_struct_t *task      = task_list_head;
    task_signal_struct_t *newtask   = (task_signal_struct_t *)
        malloc(sizeof(task_signal_struct_t));

    LOG("enqueue handler");
    // skip if list is empty
    if (task)
    {
        // iterate through list
        while (task->next && (task->taskno < taskno))
            task = task->next;

        // return if there is already a signal handler registered
        // for this task
        if (l4_task_equal(task->signal_handler, *sigthread))
        {
            LOGd(_DEBUG, "task already registered");
            return -1;
        }
    }

    newtask->signal_handler = *sigthread;
    newtask->first_signal   = NULL;
    newtask->taskno         = (*sigthread).id.task;
    newtask->waiting        = 0;
    newtask->t_alarm = ALARM_TIME_INVALID;
    newtask->taskstruct_semaphore = L4SEMAPHORE_UNLOCKED;

    if (task == NULL || (task == task_list_head && task_list_head->taskno > taskno))
        // list is empty or we have to enqueue at the beginning
    {
        newtask->next   = task;
        task_list_head  = newtask;
    }
    else // we are somewhere away from the beginning
    {
        newtask->next   = task->next;
        task->next      = newtask;
    }
    return 0;
}

void dequeue_handler( l4_threadid_t *thread )
{
    task_signal_struct_t *iterator  = task_list_head;
    task_signal_struct_t *last      = NULL;
    task_signal_struct_t *to_kill   = NULL;

    if (iterator)
    {
        // iterate over list
        while (iterator && !l4_thread_equal(iterator->signal_handler, *thread))
        {
            last = iterator;
            iterator = iterator->next;
        }

        // last is NULL if we have to dequeue the task_list_head
        if (!last)
        {
            to_kill         = iterator;
            task_list_head  = iterator->next;
        }
        // if iterator == NULL, we ran through the list without finding 
        // the thread to dequeue
        else if (iterator)
        {
            last->next      = iterator->next;
            to_kill         = iterator;
        }

        // found thread to kill?
        if (to_kill)
        {
            signal_struct_t *signal = to_kill->first_signal;
            while (signal)
            {
                to_kill->first_signal = signal->next;
                free(signal);
                signal = to_kill->first_signal;
            }
            free( to_kill );
        }
    }
}

// enqueue a signal into a task's signal queue
// taskstruct semaphore must be held
int enqueue_signal( const l4_threadid_t *thread, const siginfo_t *signal, int rec_is_thread )
{
    // malloc signal struct
    signal_struct_t *sigstruct = (signal_struct_t *)malloc(sizeof(signal_struct_t));
    // get task struct for this task
    task_signal_struct_t *task = get_task_struct( (*thread).id.task );

    // panic if no handler registered
    if (task == NULL)
    {
        LOG("no signal handler thread for this task");
        return -1;
    }

    // fill in values
    if (rec_is_thread)
        sigstruct->recipient            = *thread;
    else
        sigstruct->recipient            = L4_INVALID_ID;
    sigstruct->recipient_is_thread  = rec_is_thread;
    sigstruct->signal               = *signal;
    sigstruct->next                 = NULL;

    // enqueue
    // list is empty
    if (task->first_signal == NULL)
        task->first_signal = sigstruct;
    // list not empty
    else
    {
        signal_struct_t *it = task->first_signal;
        while (it->next)
            it = it->next;
        it->next = sigstruct;
        if (_DEBUG)
            dump_signal_list( task->first_signal );
    }

    return 0;
}

task_signal_struct_t *get_task_struct( int taskno )
{
    task_signal_struct_t *task = task_list_head;

    // iterate through list until task has been found
    while( task && task->taskno != taskno )
        task = task->next;

    return task;
}
