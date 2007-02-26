/*!
 * \file   cpu_reserve/lib/src/convenience.c
 * \brief  Convenience functions, such as a dump function
 *
 * \date   11/24/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/macros.h>
#include <l4/cpu_reserve/sched.h>
#include <l4/env/errno.h>
#include <stdlib.h>

int l4cpu_reserve_dump(void){
    int err, count, i;
    l4_threadid_t thread, creator;
    int id, prio, period, deadline, wcet;
    char *n;
    
    if((count = l4cpu_reserve_scheds_count())<0){
	l4env_perror("l4cpu_reserve_scheds_count()", -count);
	return count;
    }
    printf("CPU Reserve: %d reservations\n", count);
    for(i=0;i<count;i++){
	if((err = l4cpu_reserve_scheds_get(i, &n, &thread, &creator, &id,
					   &prio, &period, &wcet,
					   &deadline))<0){
	    l4env_perror("l4cpu_reserve_scheds_get(%d)", -err, i);
                    return 1;
	}
	if(period){
	    printf(l4util_idfmt"/%d p=%#4x T=%5d C=%5d D=%5d\n",
		   l4util_idstr(thread), id, prio, period, wcet, deadline);
	} else {
	    printf(l4util_idfmt"/%d p=%#4x delayed preemption, C=%5d\n",
		   l4util_idstr(thread), id, prio, wcet);
	}
	printf("  name=\"%s\", created by " l4util_idfmt ", nr=%d\n",
	       n, l4util_idstr(creator), i);
	free(n);
    }
    return 0;
}

