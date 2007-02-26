#include <stdlib.h>

#include <dice/dice.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>

#include "local.h"
#include "signal-server.h"

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif

const char *me = "sig_serv";
task_signal_struct_t *task_list_head = NULL;
l4_threadid_t main_thread            = L4_INVALID_ID;

int main(int argc, char**argv){

    int ret;
    CORBA_Server_Environment env = dice_default_server_environment;
    
    env.malloc  = (dice_malloc_func)malloc;
    env.free    = (dice_free_func)free;

    ret = names_register( me );
    LOGd(_DEBUG, "registered at names: %d", ret);

    task_list_head  = NULL;
    main_thread = l4_myself();

    LOGd(_DEBUG, "initializing tsc");
    l4_tsc_init(L4_TSC_INIT_AUTO);
    
    LOGd(_DEBUG, "starting alarm scheduler");
    l4thread_create(alarm_scheduler, NULL, L4THREAD_CREATE_SYNC);

    LOGd(_DEBUG, "signal server ready");
    signal_signal_server_loop(&env);
  return 0;
}
