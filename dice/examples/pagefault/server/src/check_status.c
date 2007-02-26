#include "test-server.h"

CORBA_void handler_check_status_component(CORBA_Object _dice_corba_obj,
        CORBA_long *status, CORBA_Server_Environment *_dice_corba_env)
{
    *status = 123;
}

