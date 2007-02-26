/*!
 * \file   cpu_reserve/server/src/granularity.c
 * \brief  CPU timeslice granularity functions
 *
 * \date   09/14/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/util/util.h>
#include <l4/util/kip.h>
#include <l4/env/errno.h>
#include "granularity.h"

static int kernel_granularity;

/*!\brief Obtain the timer granularity from the kip
 * \retval 0 ok
 * \retval !0 error
 */
static int granularity_init(void){
    l4_kernel_info_t *kip;
    if(kernel_granularity) return 0;

    kip = l4util_kip_map();
    if(kip==0) return -L4_EINVAL;
    kernel_granularity = kip->scheduler_granularity;
    return 0;
}

/*!\brief Return scheduling granularity
 */
int granularity(void){
    if(granularity_init()) return 0;
    return kernel_granularity;
}

/*!\brief Round time up to the next kernel-supported granularity
 *
 * \retval 0	some error occured
 * \retval >0	OK
 */
int granularity_roundup(unsigned time){
    if(!time) return 0;

    if(granularity_init()) return 0;
    time = (time-1)/kernel_granularity;
    return (time+1)*kernel_granularity;
}

