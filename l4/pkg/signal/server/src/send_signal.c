#include <signal.h>
#include <stdlib.h>

#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include "local.h"
#include "signal-server.h"

extern int _DEBUG;

void send_signal( task_signal_struct_t *task )
{
    CORBA_Server_Environment _dice_server_env = dice_default_server_environment;
    l4_threadid_t rec;
    
    signal_struct_t *sig = task->first_signal;
    rec                  = task->first_signal->recipient;
    task->first_signal   = task->first_signal->next;
    
    //LOGd(_DEBUG, "sending to: "l4util_idfmt, l4util_idstr(task->signal_handler));
    signal_signal_receive_signal_reply( &(task->signal_handler),
            sig->signal,
            &rec,
            &_dice_server_env );

    free(sig);
}
