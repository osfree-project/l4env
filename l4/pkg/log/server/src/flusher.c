/*!
 * \file	log/server/src/flusher.c
 * \brief	TCP-based Server flushing to various output media.
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * This file includes the code to flush the buffered log-data.
 * Network-specific things are handled in tcpip.c.
 *
 * The communication with the main thread is done by waiting for IPCs from
 * the main thread. Our own OSKIT-Code is activated by either our actions,
 * or by actions due to interrupts/exceptions. We notice an interrupt/exception
 * because our wait-IPC is aborted. In reaction to this, we check if something
 * happened on the net.
 *
 * In detail, this file hanles:
 * - creating the listener thread
 * - implementing the flush-operations
 *
 * Function-Interface:	flusher_init(), flush_buffer, do_flush_buffer().
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/util/util.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/rmgr/librmgr.h>
#if CONFIG_USE_SERIAL
#include <l4/serial/serial.h>
#endif
#include <assert.h>

#include <l4/log/l4log.h>
#include <stdio.h>
#include <errno.h>
#include "config.h"
#include "tcpip.h"
#include "flusher.h"
#include "stuff.h"

#if CONFIG_USE_TCPIP==0
const unsigned client_socket=0;
#endif

int flusher_prio;
l4_threadid_t main_thread, flusher_thread;


static l4_threadid_t flush_requester;

/*!\brief Wait for an IPC from the main thread.
 *
 * The main thread will do an IPC-call to indicate a full buffer and
 * wait for successful flushing..
 */
static int wait_for_flush_request(void){
    int err;
    l4_msgdope_t result;
    l4_umword_t dw0, dw1;

    err = l4_ipc_wait(&flush_requester, NULL, &dw0, &dw1,
			   L4_IPC_NEVER, &result);
    LOGd(CONFIG_LOG_IPC, "received ipc, err=%#x", err);
    return err;
}

/*!\brief Answer a flush-request from the main thread. */
static int answer_flush_request(void){
    int err;
    l4_msgdope_t result;

    do{err = l4_ipc_send(flush_requester, NULL, 0, 0,
			      L4_IPC_NEVER, &result);
    }while(err == L4_IPC_SECANCELED);
    return err;
}

/*!\brief Thread-loop: Wait for a client and handle it's requests
 *
 * This function opens the bound socket, and triggers the accept on it.
 * Then it sends the up-and-running IPC to it's parrent and waits for
 * clients to connect. If we have a client, wait for IPC's from the main
 * thread requesting a buffer-flush.
 *
 * Alternatively, this IPC may be aborted by local interrupt handling. If we
 * realize this, we check the network stack.
 *
 * As reaction to creation, we send an IPC to main_thread, dw0 contains
 * an error code, 0 means success.
 */
static void thread_loop(void){
    l4_msgdope_t result;
    int ret=0, err;

    /* raise priority */
    rmgr_set_prio(l4_myself(), flusher_prio);

#if CONFIG_USE_TCPIP
    if(flush_to_net && (ret=net_init(0,0))!=0){
	LOG_Error("Error %#x initializing network", ret);
    }
#endif

    do{
	err = l4_ipc_send(main_thread, NULL, ret, 0,
			       L4_IPC_NEVER, &result);
    } while(err==L4_IPC_SECANCELED);
    if(err){
	LOG_Error("sending ipc to main thread returned error %#x.", err);
	enter_kdebug("and now?");
    }

    /* Our acceptor socket is up and running */
    while(1){
#if CONFIG_USE_TCPIP
	if(flush_to_net){
	    err = net_wait_for_client();
	    if(err){
		LOG_Error("Error %#x waiting for next client.", err);
		l4_sleep(1000);
		continue;
	    }
	    /* We have a new client now. */
	}
#endif
        /* Wait for requests from the main thread. If the IPC is
	   aborted, this is probably due to an interrupt at the
	   network stack. We react to this in polling the
	   client-socket. */
	do{
	    err = wait_for_flush_request();
	    if(err & L4_IPC_RECANCELED){
		/* Probably we got an interrupt, check the network */
		LOGd(CONFIG_LOG_IPC,
			"Probably got interrupt, try to read from network");

		// this may result in calling do_flush_buffer
#if CONFIG_USE_TCPIP
		if(flush_to_net){
		    if((err=net_receive_check())!=0){
			LOGd(CONFIG_LOG_TCPIP,
				"net_receive_check() returned error %#x", err);
			break;
		    }
		}
#endif
	    } else if(err){ // IPC not cancelled
		    LOG_Error("Error %#x receiving IPC from main-thread", err);
		    continue;
	    } else { /* got real and valid IPC from main-thread, flush
			the buffer and send an answer */
		// LOG("begin flush");
		do_flush_buffer();
		// LOG("end flush");
		err = answer_flush_request();
		if(err){
		    LOG_Error("Error anwering flush-request.");
		}
	    }
	} while (!flush_to_net || client_socket);
    } // never ending

    LOG_Error("Log-TCP-Srv-thread: How did I get here?");
    enter_kdebug("Something went really wrong.");
}

static int general_flush_buffer(int head, int(*flush)(const char*, int)){
    int err;

    if(head < buffer_tail){
	LOGd(CONFIG_LOG_RINGBUFFER, "flushing 1/2 %d+%d",
		buffer_tail, buffer_size-buffer_tail);
	if((err=flush(buffer_array+buffer_tail,
		      (int)buffer_size-buffer_tail))!=0) return err;
	LOGd(CONFIG_LOG_RINGBUFFER, "flushing 2/2 %d+%d",
		0, head);
	if((err=flush(buffer_array, head))!=0) return err;
    } else {
	LOGd(CONFIG_LOG_RINGBUFFER, "flushing 1/1 %d+%d",
		buffer_tail, head-buffer_tail);
	err = flush(buffer_array+buffer_tail, head-buffer_tail);
    }
    return err;
}
		
/*!\brief Flush the buffer to the local console.
 *
 * \retval	0 on success, error otherwise.
 */
static int console_flush(const char*addr, int size){
    while(*addr && size--){
    	outchar(*addr++);
    }
    return 0;
}

#if CONFIG_USE_SERIAL
int serial_flush(const char*addr, int size){
    while(*addr && size--){
	int err;
	err = l4serial_outstring(addr, 1);
	if(err) printf("\nl4serial_outstring(): %d\n", err);
	if(*addr=='\n') l4serial_outstring("\r", 1);
	addr++;
    }
    return 0;
}
#endif


/*!\brief Flush the buffered data.
 *
 * Context: Flusher thread.
 *
 * net_flush_buffer() is exported from tcpip.c, console_flush() is a local
 * function.
 */
int do_flush_buffer(void){
    unsigned head = buffer_head;
    int err;

    LOGd(CONFIG_LOG_RINGBUFFER, "Flushing buffer now.");

    if(flush_local){
	err = general_flush_buffer(head, console_flush);
	if(err) return err;
    }
#if CONFIG_USE_SERIAL
    if(flush_serial){
	err = general_flush_buffer(head, serial_flush);
	if(err) return err;
    }
#endif
#if CONFIG_USE_TCPIP
    if(flush_to_net){
	err = general_flush_buffer(head, net_flush_buffer);
	if(err) return err;
    }
#endif
    buffer_tail = head;
    return 0;
}

/*!\brief Request flushing the buffer.
 *
 * This function blocks until the buffer is flushed. If we are in muxed mode
 * (flush_muxed==1), this function also flushes the binary buffers.
 *
 * Context: Main thread, flush-signaller thread
 *
 * \retval	0 on success, error otherwise.
 */
int flush_buffer(void){
    l4_msgdope_t result;
    l4_umword_t dw0, dw1;
    int err;

    if(buffer_size){
	err = L4_IPC_SECANCELED;
	while(err == L4_IPC_SECANCELED){
	    err = l4_ipc_call(flusher_thread, NULL, 0, 0,
				   NULL, &dw0, &dw1,
				   L4_IPC_NEVER, &result);
	}
	while(err == L4_IPC_RECANCELED){
	    err = l4_ipc_receive(flusher_thread, NULL, &dw0, &dw1,
				      L4_IPC_NEVER, &result);
	}
	
	if(err){
	    LOG_Error("Calling the flusher thread returned %#x.", err);
	    return err;
	}
	return dw0;
    } else {
	return 0;
    }
}


/*!\brief flush-signaller - request flushing
 *
 * This function runs at a very low priority, and requests flushing the
 * buffer, if it is non-empty.
 *
 * \pre main_thread and flusher_thread must be set.
 */
static void flush_signaller(void){

    /* Don't use 1 as priority for user level threads. The reason is that
     * current versions of Fiasco schedule all threads of each priority
     * level round robin. Since the kernel idle loop runs at priority 1 too,
     * a woken user level thread at this priority does not run until the full
     * timeslice of the idle loop is consumed. The kernel idle thread is
     * running everytime and threads which become ready are enqueued at the
     * end of the running queue. */
    rmgr_set_prio(l4_myself(), 2);
    while(1){
	if(buffer_head!=buffer_tail){
	    flush_buffer();
	}
	l4_sleep(100);		// sleep 10 msecs
    }
}

#if CONFIG_USE_SERIAL
static void serial_esc_fn(void){
    int err;

    while(1){
	err = l4serial_readbyte();
	if(err == '\e') enter_kdebug("ESC on serial");
    }
}
#endif

/*!\brief Initialize the tcp-handling stuff
 *
 * This function creates the TCP/IP handling thread.
 * Then it waits for its startup-IPC.
 *
 * Context: Main thread.
 *
 * \return	0 on success
 */
int flusher_init(void){
    l4_msgdope_t result;
    l4_umword_t dw0, dw1;
    int err;

    main_thread = l4_myself();

    /* Start the threads */
    err = thread_create(thread_loop, &flusher_thread, "log.flush");
    if(err<0){
	LOG_Error("Error %#x creating acceptor thread.", err);
	goto error;
    }
    do{
	err = l4_ipc_receive(flusher_thread, NULL, &dw0, &dw1,
				  L4_IPC_NEVER, &result);
    } while(err == L4_IPC_RECANCELED);
    if(err){
	LOG_Error("Error %#x receiving up-and-running IPC from tcpip-thread.",
	      err);
	goto error;
    }
    if(dw0!=0){
	err = dw0;
	goto error;
    }

#if CONFIG_USE_SERIAL
    if(flush_serial){
	err = l4serial_init(MAXTHREADS+1, flush_serial);
	if(err){
	    LOG_Error("Error %#x setting up the serial handler.", err);
	    goto error;
	}
	if(serial_esc){
	    l4_threadid_t dummy_t;
	    err = thread_create(serial_esc_fn, &dummy_t, "log.serialesc");
	    if(err<0){
		LOG_Error("Error %#x creating serial-esc thread", err);
		goto error;
	    }
	}
    }
#endif

    err = thread_create(flush_signaller, NULL, "log.flushsig");
    if(err){
	LOG_Error("Error %#x creating flush-signaller thread.", err);
	goto error;
    }

    return 0;

  error:
    return err;
}

