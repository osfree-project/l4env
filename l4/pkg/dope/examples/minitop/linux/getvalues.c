/*
 * \brief   DOpE mini top - retrieve timing values from Linux
 * \date    2004-01-11
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * The values provided here are just bogus - only for testing
 * under Linux.
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

/*** LOCAL INCLUDES ***/
#include "getvalues.h"


#define MAX_LISTED_THREADS 64

static struct thread_value {
	int   task_id;
	int   thread_id;
	float percent;
	long  usec;
} thread_value[MAX_LISTED_THREADS], *thread_ptr[MAX_LISTED_THREADS];

static int num;


/*** RETRIEVE TIMING VALUES FROM FIASCO ***/
void minitop_get_values(void) {
	int i;
	for (i=0; i<MAX_LISTED_THREADS; i++) {
		thread_value[i].task_id   = rand() % 42;
		thread_value[i].thread_id = rand() % 42;
		if (i<20)
			thread_value[i].usec  = rand() % 100000;
		else
			thread_value[i].usec  = 0;
		thread_value[i].percent   = (float)thread_value[i].usec / 1000.0;
		thread_ptr[i] = &thread_value[i];
	}
	num = MAX_LISTED_THREADS;
}


/*** SORT ENTRIES SO THAT BUSIEST THREADS ARE THE FIRST ***/
void minitop_sort_values(void) {
	int i,j,k;

	for (i=0; i<MAX_LISTED_THREADS-1; i++) {
		k = i;
		for (j=i; j<MAX_LISTED_THREADS; j++) {
			if (thread_ptr[j]->usec > thread_ptr[k]->usec) k=j;
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
	return thread_ptr[idx]->percent;
}


/*** REQUEST MICROSECONDS VALUE ***/
long minitop_get_usec(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	return thread_ptr[idx]->usec;
}


/*** REQUEST TASK ID ***/
int minitop_get_taskid(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	return thread_ptr[idx]->task_id;
}


/*** REQUEST THREAD ID ***/
int minitop_get_threadid(unsigned int idx) {
	if (idx >= MAX_LISTED_THREADS) return 0;
	return thread_ptr[idx]->thread_id;
}

