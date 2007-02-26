/*!
 * \file   l4env/lib/src/init.c
 * \brief  Initcall handling for l4env
 *
 * \date   11/27/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#include <l4/env/init.h>

void l4env_do_initcalls(void){
    l4env_initcall_t *fn;

    for(fn = &__l4env_initcall_start; fn < &__l4env_initcall_end; fn++){
        (*fn)();
    }
}
