/*!
 * \file	log/server/src/stuff.c
 * \brief       Log-Server, div. stuff like thread creation.
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
#include <l4/util/thread.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <stdio.h>
#include "stuff.h"
#include "flusher.h"
#include "config.h"

#if CONFIG_USE_TCPIP
/* We need heap for the net, 512KB seems to be sufficient. Define this here.
 */
oskit_addr_t l4libc_heapsize = 1024*512;
#endif

#define STACKSIZE 16384

static char stacks[MAXTHREADS][STACKSIZE];
/* Create a new thread. */
int thread_create(void(*func)(void), l4_threadid_t *id, const char*name){
    static int next_thread = 1;
    l4_umword_t *esp;
    l4_threadid_t thread = l4_myself();

    if(next_thread>MAXTHREADS){
	LOG_Error("No more space to create a thread");
	return -1;
    }

    thread.id.lthread = next_thread;
    if(id) *id = thread;

    esp = (l4_umword_t*)&stacks[next_thread][0];
    l4util_create_thread(next_thread, func, esp);
    names_register_thread_weak(name, thread);
    next_thread++;

    return 0;
}

