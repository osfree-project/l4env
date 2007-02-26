#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/util/rdtsc.h>
#include <l4/sys/types.h>

#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include "signal-client.h"
#include "local.h"

extern int _DEBUG;

void alarm_scheduler(void *data)
{
    l4thread_started(NULL);
    while (1)
    {
        // iterate over the task structs and adapt alarm times
        task_signal_struct_t *it = task_list_head;
        LOGd(_DEBUG, "alarm scheduler running...");

        while (it)
        {
            l4_cpu_time_t cur_time = l4_rdtsc();
            l4semaphore_down(&it->taskstruct_semaphore);

            // ALARM for each task with valid alarm timestamp, where t_alarm < rdtsc
            if (it->t_alarm != ALARM_TIME_INVALID &&
                cur_time > it->t_alarm)
            {
                siginfo_t sig;

                CORBA_Environment _dice_corba_env = dice_default_environment;
                _dice_corba_env.malloc = (dice_malloc_func)malloc;
                _dice_corba_env.free   = (dice_free_func)free;

                sig.si_signo = SIGALRM;

                it->t_alarm = ALARM_TIME_INVALID;

                LOG("sending SIGALRM to "l4util_idfmt,
                        l4util_idstr(main_thread));

                l4semaphore_up(&it->taskstruct_semaphore);
                signal_signal_kill_call(&main_thread,
                        &(it->signal_handler), &sig,
                        &_dice_corba_env);

            }
            else
            {
                l4semaphore_up(&it->taskstruct_semaphore);
            }

            it = it->next;
        }
        l4_sleep(1000);
    }
}
