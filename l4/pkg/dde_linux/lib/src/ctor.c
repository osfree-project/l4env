/*!
 * \file   dde_linux/lib/src/ctor.c
 * \brief  Initcall handling for l4dde
 *
 * \date   11/27/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#include <l4/dde_linux/ctor.h>

void l4dde_do_initcalls(void){
    l4dde_initcall_t *fn;

    for(fn = &__DDE_CTOR_LIST__; fn < &__DDE_CTOR_END__; fn++){
        (*fn)();
    }
}
