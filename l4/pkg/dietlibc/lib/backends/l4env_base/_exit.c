/*!
 * \file   dietlibc/lib/backends/l4env_base/_exit.c
 * \brief  
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <l4/util/util.h>

#include "_exit.h"

void _exit(int code){
    if (code) {
        printf("\nExiting with %d\n", code);
    } else {
        printf("Main function returned.\n");
    }
    l4_sleep_forever();
}

void __thread_doexit(int code)
{
    _exit(code);
    /*
      fast fix for linker error
    */
}
