/*!
 * \file   cpu_reserve/examples/resv/main.c
 * \brief  Testcase to demonstrate behavior of failing start_periodic_self()
 *
 * \date   09/04/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/sys/rt_sched.h>
#include <l4/sigma0/kip.h>
#include <l4/rmgr/librmgr.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char**argv){
    int err;
    l4_kernel_info_t *kip;
    int wcet, id;

    rmgr_init();
    kip = l4sigma0_kip_map(L4_INVALID_ID);

    wcet = 10000;
    if((err = l4cpu_reserve_add(l4_myself(), "reservation",
				0x10, 100000, &wcet, 0,
				&id))!=0){
	l4env_perror("l4cpu_reserve_add()", -err);
	return 1;
    }
    printf("Starting myself in periodic mode, time in the past.\n"
           "This must fail.\n");
    err = l4cpu_reserve_begin_strictly_periodic_self_deprecated(
	l4_myself(), kip->clock-10000);
							      
    LOGL("l4cpu_reserve_begin_strictly_periodic_self_deprecated(): %s",
	 l4env_strerror(-err));
    LOG_flush();
    enter_kdebug("Periodic mode?");

    return 0;
}
