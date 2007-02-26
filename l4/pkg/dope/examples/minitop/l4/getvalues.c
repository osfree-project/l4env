/*
 * \brief   DOpE mini top - retrieve timing values from Fiasco
 * \date    2004-01-11
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Based on l4/pkg/hello/examples/top by Frank Mehnert.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/kip.h>
#include <l4/util/util.h>
#include <l4/util/thread.h>

/*** LOCAL INCLUDES ***/
#include "getvalues.h"


#define MAX_LISTED_THREADS 512

static struct thread_value {
	l4_threadid_t  tid;
	l4_uint64_t    new_us;
	l4_uint64_t    old_us;
	l4_uint64_t    curr_us;
	l4_umword_t    prio;
} thread_value[MAX_LISTED_THREADS], *thread_ptr[MAX_LISTED_THREADS];

static l4_uint64_t us, total_old_us, total_new_us, total_curr_us;
static int num;


/*** RETRIEVE TIMING VALUES FROM FIASCO ***/
void minitop_get_values(void) {
	l4_threadid_t tid;
	l4_threadid_t next_tid;
	l4_umword_t prio;

	static int warn_limit = 10;   /* max. number of warnings to print out */

	/* init index pointers */
	for(num=0; num<MAX_LISTED_THREADS; num++)
		thread_ptr[num] = &thread_value[num];

	tid = L4_NIL_ID;
	num = 0;
	total_old_us = total_new_us;
	total_new_us = 0;

	do {
		if (fiasco_get_cputime(tid, &next_tid, &us, &prio) != 0) {
			
			/* Error */
			if (l4_is_nil_id(tid)) {
				/* very strange: no accounting info for idle thread? */
				printf("no accounting information for idle thread?");
				exit(-3);
			}
			return;
		}
		thread_value[num].tid = tid;
		thread_value[num].old_us = thread_value[num].new_us;
		thread_value[num].new_us = us;
		thread_value[num].prio = prio;
		thread_value[num].curr_us = thread_value[num].new_us - thread_value[num].old_us;

		total_new_us += us;
		tid = next_tid;

		if (++num >= MAX_LISTED_THREADS) {
			if (warn_limit > 0) {
				printf("Warning: too many threads, limit is %d\n", MAX_LISTED_THREADS);
				warn_limit--;
			}
			break;
		}

	} while (!l4_is_nil_id(tid));
	total_curr_us = total_new_us - total_old_us;
}


/*** SORT ENTRIES SO THAT BUSIEST THREADS ARE THE FIRST ***/
void minitop_sort_values(void) {
	int i,j,k;

	for (i=0; i<MAX_LISTED_THREADS-1; i++) {
		k = i;
		for (j=i; j<MAX_LISTED_THREADS; j++) {
			if (thread_ptr[j]->curr_us > thread_ptr[k]->curr_us) k=j;
		}
		if (k != i) {
			struct thread_value *dummy;
			dummy         = thread_ptr[i];
			thread_ptr[i] = thread_ptr[k];
			thread_ptr[k] = dummy;
		}
	}
}


/*** REQUEST CURRENT NUMBER OF TABLE ENTRIES ***/
int minitop_get_num(void) {
	return num;
}


/*** REQUEST PERCENT CPU LOAD ***/
float minitop_get_percent(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	if (total_curr_us == 0) return 0;
	return (float)(thread_ptr[idx]->curr_us)
	       * 100.0 / (float)total_curr_us;
}


/*** REQUEST MICROSECONDS VALUE ***/
long minitop_get_usec(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	return thread_ptr[idx]->curr_us;
}


/*** REQUEST TASK ID ***/
int minitop_get_taskid(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	return thread_ptr[idx]->tid.id.task;
}


/*** REQUEST THREAD ID ***/
int minitop_get_threadid(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	return thread_ptr[idx]->tid.id.lthread;
}

