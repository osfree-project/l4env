/**
 * \file   libc_backends_l4env/lib/l4env_start_stop/_exit.c
 * \brief  Implementation for _exit() and __thread_doexit().
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>

#include <l4/crtx/crt0.h>
#include <l4/generic_ts/generic_ts.h>
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

    if (! l4ts_connected())
    {
        printf("SIMPLE_TS not found -- cannot send exit event");
        crt0_sys_destruction();
        l4_sleep_forever();
    }
    else
    {
        crt0_sys_destruction();
        l4ts_exit();
    }
}

void __thread_doexit(int code)
{
    /*
      fast fix for linker error
    */
}
