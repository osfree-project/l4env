#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/signal/signal-client.h>

#include "local.h"

extern l4_threadid_t l4signal_signal_server_id;

unsigned int alarm(unsigned int seconds)
{
    l4_threadid_t me = l4_myself();
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc      = (dice_malloc_func)malloc;
    _dice_corba_env.free        = (dice_free_func)free;

    return signal_signal_alarm_call(&l4signal_signal_server_id,
            &me, seconds, &_dice_corba_env);
}
