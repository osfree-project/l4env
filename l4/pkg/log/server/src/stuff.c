/*!
 * \file	log/server/src/stuff.c
 * \brief       Log-Server, div. stuff like locking and thread creation.
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/wait_queue.h>
#include <l4/log/l4log.h>
#include <stdio.h>
#include "stuff.h"
#include "flusher.h"
#include "config.h"

#if CONFIG_USE_TCPIP
#include <oskit/lmm.h>
#else
#include <flux/lmm.h>
#endif

#if CONFIG_USE_TCPIP
/* We need heap for the net, 512KB seems to be sufficient. Define this here.
 */
oskit_addr_t l4libc_heapsize = 1024*512;
#endif

#define STACKSIZE 16384
#define MAXTHREADS 2
static l4_wait_queue_t buffer_lock_wq=L4_WAIT_QUEUE_INITIALIZER;

void logserver_lock(void){
    LOGd_Enter(CONFIG_LOG_LOCK);
    l4_wq_lock(&buffer_lock_wq);
    LOGd(CONFIG_LOG_LOCK, "locked\n");
}
void logserver_unlock(void){
    LOGd_Enter(CONFIG_LOG_LOCK);
    l4_wq_unlock(&buffer_lock_wq);
    LOGd(CONFIG_LOG_LOCK, "unlocked\n");
}

static char stacks[MAXTHREADS][STACKSIZE];
/* Create a new thread. */
int thread_create(void(*func)(void*), void*data, l4_threadid_t *id){
    static int next_thread = 1;
    l4_umword_t dummy, *esp;
    l4_threadid_t thread = l4_myself();
    l4_threadid_t preempter = L4_INVALID_ID, 
	pager = L4_INVALID_ID;

    if(next_thread>MAXTHREADS){
	LOG_Error("No more space to create a thread");
	return -1;
    }

    /* get my own preempter and pager */
    l4_thread_ex_regs(l4_myself(), -1, -1, &preempter, &pager,
		      &dummy, &dummy, &dummy);
    if(l4_is_invalid_id(pager)){
	LOG_Error("I have no pager!");
	return -2;  // no pager!
    }
  
    thread.id.lthread = next_thread;
    if(id) *id = thread;

    esp = (l4_umword_t*)&stacks[next_thread][0];

    /* put some parameters on the stack */
    *--esp = (l4_umword_t) data;
    *--esp = 0;             // kind of return address
      
    l4_thread_ex_regs(thread, (l4_umword_t)func, 
		      (l4_umword_t) esp, &preempter, &pager,
		      &dummy, &dummy, &dummy);
                      
    next_thread++;

    return 0;
}

