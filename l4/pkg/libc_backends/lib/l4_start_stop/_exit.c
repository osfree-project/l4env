/**
 * \file   dietlibc/lib/backends/l4_start_stop/_exit.c
 * \brief  
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
