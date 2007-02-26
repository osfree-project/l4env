#include <stdio.h>
#include <l4/crtx/crt0.h>
#include <l4/util/util.h>

#include "_exit.h"

void _exit(int code)
{
    if (code)
    {
        printf("\nExiting with %d\n", code);
    }
    else
    {
        printf("Main function returned.\n");
    }

    crt0_sys_destruction();
    l4_sleep_forever();
}

void __thread_doexit(int code)
{
    /*
      fast fix for linker error
    */
}
