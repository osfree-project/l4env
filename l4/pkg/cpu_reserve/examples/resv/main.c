/*!
 * \file   cpu_reserve/examples/resv/main.c
 * \brief  Client app doing some reservations
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
#include <l4/util/parse_cmd.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char**argv){
    int prio, deadline, wcet, period, count;
    int err, i;
    l4_threadid_t me = l4_myself();

    if(parse_cmdline(&argc, &argv,
		     'T', "period", "period",
		     PARSE_CMD_INT, 10000, &period,
		     'p', "prio", "priority (for all timeslices)",
		     PARSE_CMD_STRING, 130, &prio,
		     'D', "deadline", "deadline (for each timeslice)",
		     PARSE_CMD_INT, 10000, &deadline,
		     'C', "wcet", "wcet",
		     PARSE_CMD_INT, 3000, &wcet,
		     'c', "count", "number of reservations of this type",
		     PARSE_CMD_INT, 1, &count,
		     0, 0)) return 1;

    for(i=0; i<count;i++){
	int id;
	printf("Doing reservation %d: p=%d T=%d C=%d D=%d\n",
	       i+1, prio, period, wcet, deadline);
	if((err = l4cpu_reserve_add(me,
				    "reservation",
				    prio,
				    period,
				    &wcet,
				    deadline,
				    &id))!=0){
	    l4env_perror("l4cpu_reserve_add()", -err);
	    return 1;
	}
	printf("  id: %d, reserved C=%d\n", id, wcet);
    }
    return 0;
}
