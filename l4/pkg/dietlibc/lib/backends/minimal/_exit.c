#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include <stdio.h>

#include "_exit.h"

void exit_sleep(void)
{
    for(;;)
    {
        l4_msgdope_t result;
        l4_umword_t dummy;
        l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
                            L4_IPC_NEVER, &result);
    }
}

void
_exit(int code)
{
    if (code)
    {
        printf("\nExiting with %d\n", code);
        exit_sleep();
    }
    else
    {
        printf("Main function exit with code %d\n", code);
        exit_sleep();
    }
}

void __thread_doexit(int code)
{
    /*
      fast fix for linker error
    */
}
