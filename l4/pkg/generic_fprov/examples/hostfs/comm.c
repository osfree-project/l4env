/*!
 * \file   generic_fprov/examples/hostfs/comm.c
 * \brief  communication with oskit's fs thread
 *
 * \date   07/28/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * We need this intermediate layer as OSKit relies on interrupt-propagation
 * using exceptions. However, these exceptions disturb our IPCs used for
 * everything in DROPS. And nobody implemented safe restart of IPCs.
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/l4int.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <oskit/c/assert.h>
#include "fs.h"

static struct communication_union{
    enum {
	CMD_INIT,
	CMD_DONE,
	CMD_OPEN,
	CMD_READ,
	CMD_CLOSE,
    } cmd;
    union{
	struct {
	    const char*diskname;
	    const char*partname;
	} init;
	struct {
	    const char *name;
	    long long *size;
	} open;
	struct {
	    struct fs_file_t *file;
	    long long offset;
	    char *addr;
	    l4_size_t count;
	} read;
	struct {
	    struct fs_file_t* file;
	} close;
    }c;
    union{
	int i;
	fs_file_t *f;
    }r;
} params;

static l4thread_t fs_thread;
static l4_threadid_t main_id;
static l4_threadid_t fs_thread_id;

static void dispatcher(void*);

extern inline void call_fs_thread(void);
extern inline void call_fs_thread(void){
    int err = L4_IPC_SEABORTED;
    l4_umword_t dummy;
    l4_msgdope_t res;

    do{
	err = l4_ipc_call(fs_thread_id, L4_IPC_SHORT_MSG, 0, 0,
			       L4_IPC_SHORT_MSG, &dummy, &dummy, L4_IPC_NEVER,
			       &res);
    }while((err&0x7f) == L4_IPC_SECANCELED);

    while((err&0x7f) == L4_IPC_RECANCELED){
	err = l4_ipc_receive(fs_thread_id,
				  L4_IPC_SHORT_MSG, &dummy, &dummy,
				  L4_IPC_NEVER, &res);
    }

    assert(err==0);
}

int fs_init(const char*diskname, const char*partname){
    main_id = l4_myself();
    params.cmd = CMD_INIT;
    params.c.init.diskname=diskname;
    params.c.init.partname=partname;

    if((fs_thread = l4thread_create(dispatcher, 0, L4THREAD_CREATE_ASYNC))<0){
	LOG_Error("fs_init(): %s", l4env_errstr(fs_thread));
	return fs_thread;
    }
    fs_thread_id = l4thread_l4_id(fs_thread);

    call_fs_thread();

    if(params.r.i){
	/* nothing happened at the thread, we can kill it */
	l4thread_shutdown(fs_thread);
    }
    return params.r.i;
}

int fs_done(void){
    params.cmd = CMD_DONE;
    call_fs_thread();
    if(params.r.i){
	LOG_Error("Ouch, filesystem thread reported errors on closing.");
    }
    l4thread_shutdown(fs_thread);
    return params.r.i;
}

struct fs_file_t* fs_open(const char*name, long long*size){
    params.cmd = CMD_OPEN;
    params.c.open.name = name;
    params.c.open.size = size;
    call_fs_thread();
    return params.r.f;
}

int fs_read(struct fs_file_t *file, long long offset, char*addr,
	    l4_size_t count){
    params.cmd = CMD_READ;
    params.c.read.file = file;
    params.c.read.offset = offset;
    params.c.read.addr = addr;
    params.c.read.count = count;
    call_fs_thread();
    return params.r.i;
}

int fs_close(struct fs_file_t*file){
    params.cmd = CMD_CLOSE;
    params.c.close.file = file;
    call_fs_thread();
    return params.r.i;
}

/* The poor dispatcher thread
 */
static void dispatcher(void*data){
    l4_umword_t dummy;
    l4_msgdope_t res;
    int err=L4_IPC_REABORTED;

    while(1){
	while((err&0x7f)==L4_IPC_RECANCELED){
	    err = l4_ipc_receive(main_id,
				      L4_IPC_SHORT_MSG, &dummy, &dummy,
				      L4_IPC_NEVER, &res);
	}
	if(err)LOGL("err=%#x", err);
	assert(err==0);
	switch(params.cmd){
	case CMD_INIT:
	    params.r.i = fs_do_init(params.c.init.diskname,
				    params.c.init.partname);
	    break;
	case CMD_DONE:
	    params.r.i = fs_do_done();
	    break;
	case CMD_OPEN:
	    params.r.f = fs_do_open(params.c.open.name, params.c.open.size);
	    break;
	case CMD_READ:
	    params.r.i = fs_do_read(params.c.read.file, params.c.read.offset,
				    params.c.read.addr, params.c.read.count);
	    break;
	case CMD_CLOSE:
	    params.r.i = fs_do_close(params.c.close.file);
	    break;
	}
	do{
	    err = l4_ipc_call(main_id, L4_IPC_SHORT_MSG, 0, 0,
				   L4_IPC_SHORT_MSG, &dummy, &dummy,
				   L4_IPC_NEVER, &res);
	}while((err&0x7f) == L4_IPC_SECANCELED);
	assert(err==0 || (err&0x7f) == L4_IPC_RECANCELED);
    }
}
