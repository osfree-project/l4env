#include <stdlib.h>

#include <dice/dice.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>

#include "signal-server.h"
#include "local.h"

extern int _DEBUG;

CORBA_int
signal_signal_kill_component(CORBA_Object _dice_corba_obj,
                             const l4_threadid_t *thread,
                             const siginfo_t *signal,
                             CORBA_Server_Environment *_dice_corba_env)
{
    int ret = 0;

    task_signal_struct_t *task = get_task_struct((*thread).id.task);

    l4semaphore_down(&task->taskstruct_semaphore);
    enqueue_signal( thread, signal, 0 );
    l4semaphore_up(&task->taskstruct_semaphore);


    // if we receive a SIGKILL, we want the signal server to try to
    // terminate the signaled task without cooperation. Therefore
    // we try to kill the task via the simple_ts. If that fails (the task
    // was not started by simple_ts then), we try rmgr. If rmgr also
    // fails, our only hope is cooperation of the task
    if (signal->si_signo == SIGKILL)
    {
        int r;
        int nr = (*thread).id.task;
        l4_taskid_t taskid;

        LOGd(_DEBUG, "SIGKILL received");

        // determine task_id of thread
        r = l4ts_taskno_to_taskid(nr,&taskid);
        if (r)
            LOG("error getting task id: %d (%s)", r, l4env_errstr(-r));
        LOGd(_DEBUG,"taskno: %d, killing NOW",nr);

        // try to kill task via simple_ts
        r = l4ts_kill_task(taskid,0);
        // if this doesn't work, try to kill task via rmgr
        if (r)
        {
            LOGd(_DEBUG, "could not kill via task_server.");
            r = rmgr_free_task(nr);
        }
        // if this still doesn't work, we cannot kill the task without
        // its cooperation
        if (r)
        {
            task_signal_struct_t *task = get_task_struct( (*thread).id.task );
            LOGd(_DEBUG, "could not kill via rmgr.");
            if (task->waiting)
            {
                task->waiting = 0;
                send_signal(task);
                return 0;
            }
            // no else{} because the signal is already enqueued into the
            // signal list and will be received whenever the task becomes
            // waiting again
        }
        ret = 0;
    }
    else
    {
        task_signal_struct_t *task = get_task_struct( (*thread).id.task );
        //LOGd(_DEBUG, "kill for: "l4util_idfmt, l4util_idstr(*thread));
        // check if task is waiting (and the signal was correct)
        if (ret==0 && task->waiting)
        {
            task->waiting = 0;
            send_signal( task );
        }
        // no else{} because the signal is already enqueued into the
        // signal list and will be received whenever the task becomes
        // waiting again
    }
    return ret;
}

CORBA_int
signal_signal_thread_kill_component(CORBA_Object _dice_corba_obj,
                                    const l4_threadid_t *thread,
                                    const siginfo_t *signal,
                                    CORBA_Server_Environment *_dice_corba_env)
{
    int ret = 0;

    task_signal_struct_t *task = get_task_struct( (*thread).id.task );

    l4semaphore_down(&task->taskstruct_semaphore);
    ret = enqueue_signal( thread, signal, 1 );
    l4semaphore_up(&task->taskstruct_semaphore);

    if (ret==0 && task->waiting)
    {
        task->waiting = 0;
        send_signal( task );
    }

    return ret;
}

siginfo_t
signal_signal_receive_signal_component(CORBA_Object _dice_corba_obj,
                                       l4_threadid_t *thread,
                                       CORBA_short *_dice_reply,
                                       CORBA_Server_Environment *_dice_corba_env)
{
    task_signal_struct_t *task  = get_task_struct( (*_dice_corba_obj).id.task );
    signal_struct_t *sig        = NULL;
    siginfo_t retval;

    // check whether there is an entry in signal list.
    if (task == NULL)
    {
        // PANIC!!! We do not know the sighandler for this task.
        // This must not happen.
        LOG("receive_signal() for unknown task.");
        // FIXME: what should we return here???
        return retval;
    }

    // check whether the requesting thread is the registered signal
    // handler thread
    if (!l4_thread_equal( *_dice_corba_obj, task->signal_handler ))
    {
        LOG("requester is not the registered signal handler");
        // FIXME: what should we return here???
        return retval;
    }

    // no pending signals?
    if (task->first_signal == NULL)
    {
        // we'll call you back later...
        *_dice_reply    = DICE_NO_REPLY;
        task->waiting   = 1;
        return retval;
    }

    // Don't ask. This is just to beautify the code :)
    *_dice_reply        = DICE_NO_REPLY;
    send_signal( task );

    free(sig);
    return retval;
}

CORBA_int
signal_signal_register_handler_component(CORBA_Object _dice_corba_obj,
                                         CORBA_Server_Environment *_dice_corba_env)
{
    int ret =  enqueue_handler( _dice_corba_obj );
    LOGd(_DEBUG, "registered handler: %d", ret);
    if (_DEBUG)
        dump_task_list();

    return ret;
}

CORBA_int
signal_signal_unregister_handler_component(CORBA_Object _dice_corba_obj,
                                         CORBA_Server_Environment *_dice_corba_env)
{
    LOGd(_DEBUG, "unregistering handler.");
    dequeue_handler( _dice_corba_obj );
    if (_DEBUG)
        dump_task_list();

    return 0;
}

l4_int32_t signal_signal_alarm_component(CORBA_Object _dice_corba_obj,
                            const l4_threadid_t *thread,
                            l4_int32_t seconds,
                            CORBA_Server_Environment *_dice_corba_env)
{
    int last_seconds = 0;
    l4_uint64_t new_timestamp = 0;

    task_signal_struct_t *task = get_task_struct((*_dice_corba_obj).id.task);

    if (task == NULL)
    {
        LOG("Panic! Requesting alarm for unregistered task.");
        return -1;
    }

    LOGd(_DEBUG, "Alarm in %d seconds.", seconds);

    new_timestamp = seconds;
    new_timestamp = l4_ns_to_tsc(new_timestamp * 100000000);
    LOGd(_DEBUG,"wait time: %llu", new_timestamp);

    l4semaphore_down(&task->taskstruct_semaphore);
    last_seconds = l4_tsc_to_us(task->t_alarm) / 1000;
    task->t_alarm = l4_rdtsc() + new_timestamp;
    LOGd(_DEBUG,"time   : %llu", l4_rdtsc());
    LOGd(_DEBUG,"t_alarm: %llu", task->t_alarm);
    l4semaphore_up(&task->taskstruct_semaphore);

    return last_seconds;
}
