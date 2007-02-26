/*!
 * \file   omega0/server/src/irq_threads.c
 * \brief  IRQ threads handling client requests and IRQs
 *
 * \date   2000/02/08
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2000-2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/util/util.h>
#include <l4/rmgr/librmgr.h>
#include <l4/log/l4log.h>
#include <stdlib.h>
#include <stdio.h>
#include <l4/omega0/client.h>
#include <l4/util/rdtsc.h>
#include <l4/util/spin.h>
#include <l4/util/macros.h>
#include "pic.h"
#include "globals.h"
#include "irq_threads.h"
#include "create_threads.h"	/* krishna: separate thread creation */
#include "config.h"


/*!\brief return codes for IPC handling functions */
enum client_ipc_enum {
    RET_HANDLE,		/* given ipc structure is filled with new
			 * request */
    RET_REPLY,		/* return an IPC to the caller */
    RET_WAIT,		/* return no IPC to the caller, just wait for
			 * the new request */
};

typedef struct{
    l4_threadid_t sender;
    l4_umword_t d0;
    omega0_request_t request;
} client_ipc_t;

static void attach_irq(int);

/*!\brief Signal a single user thread. Assume, the client is waiting.
 *
 * \param  ipc		If !0, the last client-reply will await a new request.
 *			IPC will contain newly-arrived client-request uppon
 *			return.
 *
 * \retval 0		OK. If ipc was !0, ipc contains the new client request.
 * \retval !0		ipc was not 0, and IPC-operation failed. Either send
 *			or receive. Anyway, don't use ipc.
 */
static int signal_single_user_thread(int irq, client_chain *c,
				     client_ipc_t *ipc){
    int error;
    l4_msgdope_t result;

    LOGdl(OMEGA0_DEBUG_IRQ_THREADS, "Got irq %#x, Wakeup thread "l4util_idfmt,
	  irq,  l4util_idstr(c->client));

    if(ipc){
	error = l4_ipc_reply_and_wait(c->client,
				      (void*)(L4_IPC_SHORT_MSG |
					      L4_IPC_DECEIT_MASK),
				      irq+1,
				      OMEGA0_DEBUG_MEASUREMENT_SENDTIME
				      ?(unsigned)(l4_rdtsc())
				      :0,
				      &ipc->sender, L4_IPC_SHORT_MSG,
				      &ipc->d0, &ipc->request.i,
				      L4_IPC_SEND_TIMEOUT_0, &result);
    } else {
	error = l4_ipc_send(c->client,
			    (void*)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK),
			    irq+1,
			    OMEGA0_DEBUG_MEASUREMENT_SENDTIME
			    ?(unsigned)(l4_rdtsc())
			    :0,
			    L4_IPC_NEVER, &result);
    }
  
    // if the ipc was successful, mark the irq as 'in service'
    if(!L4_IPC_SND_ERROR(result)){
	/* increment the number of waiting clients. We need this for the
	   automatic consume operation. */
	if(OMEGA0_STRATEGY_AUTO_CONSUME) irqs[irq].clients_waiting++;
	c->in_service = 1;
	c->last_irq_num= irqs[irq].counter;
    }
    else LOGdl(OMEGA0_DEBUG_IRQ_THREADS,
	       "error %#x sending IPC to " l4util_idfmt, 
	       error, l4util_idstr(c->client));
    c->waiting = 0;

    return error;
}

/*!\brief Signal all user threads waiting on this irq and await new request
 *
 * \note This does not allow multiple lines per irq up to now,
 *       because this requires multiple threads per client at the
 *       server.
 *
 * \param  ipc		If !0, the last client-reply will await a new request.
 *			IPC will contain newly-arrived client-request uppon
 *			return.
 *
 * \retval 0		OK. If ipc was !0, ipc contains the new client request.
 * \retval !0		ipc was not 0, and IPC-operation failed, or none was
 *			notified. Either send receive. Anyway, don't use ipc.
 */
static int signal_user_threads(int irq, client_ipc_t *ipc){
    client_chain *c;
    l4util_wq_lock_queue_elem_t wqe;

    aquire_mutex(&irqs[irq].mutex, &wqe);

    irqs[irq].counter++;
    if(OMEGA0_STRATEGY_AUTO_CONSUME) irqs[irq].clients_waiting=0;
  
    for(c=irqs[irq].clients;c;c=c->next){
	if(c->next==0 && c->waiting){
	    /* last wakeup: use IPC call */
	    release_mutex(&irqs[irq].mutex, &wqe);
	    return signal_single_user_thread(irq, c, ipc);
	} else if(c->waiting){
	    signal_single_user_thread(irq, c, 0);
	} else {
	    LOGdl(OMEGA0_DEBUG_IRQ_THREADS,
		  "client "l4util_idfmt" not waiting",
		  l4util_idstr(c->client));
	}
    }
  
    release_mutex(&irqs[irq].mutex, &wqe);

    /* we did not get a new request yet */
    return 1;
}

/*!\brief Actually consume an irq. 
 *
 * The function checks if the IRQ is in service, and if so, it
 * consumes it according to the current policy. After that, the
 * in-service flag is reset.
 *
 * \return 0 if the IRQ was in service, -1 else.
 */
static inline int irq_consume(int irq){
    // irq in service?
    if(!irqs[irq].in_service) return -1;

    /* it is acknowledged already, but implicitely masked */
    if(OMEGA0_STRATEGY_IMPLICIT_MASK){
	irq_unmask(irq);
    } else {
	irq_ack(irq);
    }
    irqs[irq].in_service = 0;
    return 0;
}

#if OMEGA0_STRATEGY_AUTO_CONSUME
/*!\brief Checks if the given client is a candidate for auto-consuming an irq.
 *
 * Auto-consuming is possible, if:
 * - the client is marked as in service (got a notification)
 * - the clients's occurence-number matches that of the IRQ
 * - the IRQ is in service
 * - this client was the last (i.e., the number if waiting clients = 1)
 *
 * Maybe some of the conditions follow from others.
 *
 * \pre The irq-mutex must be hold outside.
 *
 */
void check_auto_consume(int irq, client_chain *c){

    if(c->in_service && c->last_irq_num == irqs[irq].counter 
       && irqs[irq].in_service && --irqs[irq].clients_waiting==0){

	LOGdl(OMEGA0_DEBUG_IRQ_THREADS,
	      "Automatically consuming irq %#x", irq);
	if(irq_consume(irq)){
	    LOGl("An error occured during automatically consume operation");
	}
    } else {
	LOGdl(OMEGA0_DEBUG_IRQ_THREADS,
	      "No automatic-consume of irq %#x, clients_waiting %d now",
	      irq, irqs[irq].clients_waiting);
    }
}
#endif /* OMEGA0_STRATEGY_AUTO_CONSUME */

/*!\brief Handle clients requests.
 *
 * Actions:
 * - wait		- wait until next irq
 * - mask		- mask the (unmasked) irq
 * - unmask		- unmask the (masked) irq
 * - consume		- previously occured irq consumed, may occur again
 *
 * If an wait operation occures while the in_service flag is set, the
 * client cannot handle the irq. Do nothing. At least one client
 * should send the consume action.
 *
 * If the OMEGA0_STRATEGY_AUTO_CONSUME option is set, a consume
 * operation is automatically done if all clients are waiting an no
 * client consumed the irq explicitely (i.e., sent a consume action).
 *
 * Requests from unregistered clients are ignored silently.
 *
 * If a client has to be awaken, this function uses reply-and wait and
 * awaits the next request.
 * 
 * \param  ipc		in: contains client id and its dwords.
 *			out: contains ipc parameters of newly arrived
 *			request (if retval=RET_HANDLE)
 *
 * \retval RET_HADNLE	#ipc contains the next request. Handle it.
 * \retval RET_REPLY	Return an ipc to the client and wait for next request.
 * \retval RET_WAIT	No additional action needed, wait for next request.
 * \retval <0	error, send back to client and wait for next request.
*/
static int handle_user_request(int irq,
			       client_ipc_t *ipc){
    client_chain *c;
    int ret;
    l4util_wq_lock_queue_elem_t wqe;

    aquire_mutex(&irqs[irq].mutex, &wqe);
  
    // find the corresponding client structure
    for(c=irqs[irq].clients;c;c=c->next){
	if(l4_thread_equal(ipc->sender, c->client)){

	    if(ipc->request.s.again){
		ipc->request.s.again=0;
		ipc->request.i |= c->last_request.i;
	    }
	    // does the client want to unmask the irq and has it masked?
	    if(ipc->request.s.unmask){
		/* does the client unmask the correct irq? 
		 * (currently: only <irq>) */
		if(ipc->request.s.param!=irq+1) {
		    ret = -1; goto back;
		}
		// is it unmasked?
		if(! c->masked) {
		    ret = -2; goto back;
		}
		/* then the irq is masked. is it masked exactly once? */
		if(--irqs[irq].masked==0) irq_unmask(irq);
		ipc->request.s.unmask=0;
		c->masked = 0;
	    }
    
	    // did the client consume the irq?
	    if(ipc->request.s.consume){
		// is it not in service?
		if(!c->in_service) {
		    ret = -3; goto back;
		}
		if(irq_consume(irq)){
		    ret = -4; goto back;
		}
      		ipc->request.s.consume=0;
		c->in_service = 0;
	    }
    
	    // masks the client the irq?
	    if(ipc->request.s.mask){
		// correct irq? (currently: only <irq>)
		if(ipc->request.s.param!=irq+1) {
		    ret = -5; goto back;
		}
    
		// is it masked already?
		if(c->masked) {
		    ret = -6; goto back;
		}
      
		// is the irq unmasked now?
		if(irqs[irq].masked==0) irq_mask(irq);
		irqs[irq].masked++;
		ipc->request.s.mask=0;
		c->masked = 1;
	    }
    
	    // does the client want an IPC on next occurence of irq?
	    if(ipc->request.s.wait){
		if(OMEGA0_STRATEGY_AUTO_CONSUME){
		    check_auto_consume(irq, c);
		}

		/* if in_service flag was set, the driver is not shure it 
		 * can handle the irq */
		c->in_service = 0;
		c->waiting = 1;

		/* IRQ not consumed and not signalled to this thread ?
		 * This happens in non-auto-consume mode or in case
		 * of send-errors. */
		if(irqs[irq].in_service &&
		   irqs[irq].counter != c->last_irq_num){

		    /* Signal client and await the next request */
		    release_mutex(&irqs[irq].mutex, &wqe);
		    ret = signal_single_user_thread(irq, c, ipc)
                            ? RET_WAIT: RET_HANDLE;
		    c->last_request = ipc->request;
		    return ret;
		}
		ipc->request.s.wait=0;
		ret = RET_WAIT;		// no ipc back, wait for next request
	    } else {
		ret = RET_REPLY;	// send ipc back
	    }

	    goto back;	// break the loop
	}
    }  
    // client not found in the list, ignore the ipc
    LOGdl(OMEGA0_DEBUG_WARNING,
	  "Client "l4util_idfmt" not found in the list of attached clients, "
	  "ignoring request", l4util_idstr(ipc->sender));
    release_mutex(&irqs[irq].mutex, &wqe);
    return RET_WAIT;
  
  back:
    c->last_request = ipc->request;
    release_mutex(&irqs[irq].mutex, &wqe);
  
    LOGd(OMEGA0_DEBUG_IRQ_THREADS, 
	 "returning %d, forcing %s IPC back", ret, ret?"1":"no");
    return ret;
}

/*!\brief Attach to a specified irq. 
 *
 * If the irq is available, its available flag is set to 1, otherwise
 * it is cleared.
 */
static void attach_irq(int num){
    int error;
    l4_msgdope_t result;
    l4_umword_t dummy;
    l4_threadid_t irq_th;
  
    if(rmgr_get_irq(num) != 0){
	LOGdl(OMEGA0_DEBUG_IRQ_THREADS,
	      "error getting irq %#x from rmgr", num);
	irqs[num].available = 0;
	return;
    }
 
    l4_make_taskid_from_irq(num, &irq_th);
  
    error = l4_ipc_receive(irq_th, 0, &dummy, &dummy,
			   L4_IPC_BOTH_TIMEOUT_0, &result);
  
    if(error==L4_IPC_RETIMEOUT){
	irqs[num].available=1;
	irqs[num].shared=0;
	irqs[num].masked=0;
	irqs[num].clients=NULL;
	irqs[num].counter=0;
	irqs[num].clients_waiting = 0;
	irqs[num].clients_registered = 0;
	irqs[num].mutex.last = NULL;
    } else {
	irqs[num].available=0;
    }
}

/*!\brief Handler function for one irq.
 *
 * After calling attach_irq(), return an IPC to the management thread
 * and wait for incoming IRQs.
 */
void irq_handler(int num){
    int error;
    l4_msgdope_t result;
    l4_threadid_t mainthread = l4_myself();
    client_ipc_t ipc;

    /* krishna: setup lthread info */
    irqs[num].lthread = mainthread.id.lthread;

    mainthread.id.lthread = MANAGEMENT_THREAD;
    attach_irq(num);
    if(irqs[num].available){
	LOGdl(OMEGA0_DEBUG_IRQ_THREADS, "irq %#02x available", num);
    } else {
	LOGdl(OMEGA0_DEBUG_IRQ_THREADS, "irq %#02x not available", num);
    }
  
    do{
	error = l4_ipc_send(mainthread, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_NEVER,
			    &result);
	if(error){
	    LOGL("error sending birth-ipc to "l4util_idfmt".", 
		 l4util_idstr(mainthread));
	    enter_kdebug("!");
	}
    }while(error);


    /* wait for ipcs now */
    error = l4_ipc_wait(&ipc.sender, L4_IPC_SHORT_MSG,
			&ipc.d0, &ipc.request.i, L4_IPC_NEVER, &result);
    while(1){
	while(error){
	    LOGdl(OMEGA0_DEBUG_WARNING,
		  "IRQ %d: ipc error %#x", num, error);
	    error = l4_ipc_wait(&ipc.sender, L4_IPC_SHORT_MSG,
				&ipc.d0, &ipc.request.i,
				L4_IPC_NEVER, &result);
	}
	
	if(ipc.sender.lh.high==0 && ipc.sender.lh.low<=IRQ_NUMS){
	    /* irq from kernel */
	    
	    if(OMEGA0_DEBUG_IRQ_THREADS) l4_spin_text(0, num, "IRQ: ");
	    
	    irqs[num].in_service = 1;
	    if(num == 7 && OMEGA0_DEBUG_IRQ_THREADS) {
		l4_spin_text(0,14,"IRQ 7: ");
	    }
	    
	    /* implicitely mask the irq */
	    if(OMEGA0_STRATEGY_IMPLICIT_MASK){
		/* mask and acknowledge the irq at the pic */
		irq_mask_and_ack(num);
	    
		LOGd(OMEGA0_DEBUG_PIC,
		     "irq=%x, ISR after ack=%#x, IRR=%#x",
		     num, pic_isr(num>=8), pic_irr(num>=8));
	    }
	    
	    /* signal all waiting threads now and wait for new
	     * request. */
	    error = signal_user_threads(num, &ipc);
	} else {
	    int ret;
	    /* ipc from user thread, must be a request */
	    if(ipc.d0!=OMEGA0_REQUEST){
		LOGd(OMEGA0_DEBUG_REQUESTS,
		     "non-Omega0 request from "l4util_idfmt,
		     l4util_idstr(ipc.sender));
		error = 1;
		continue;
	    }
	    LOGd(OMEGA0_DEBUG_REQUESTS,
		 "request %#08lx from "l4util_idfmt" (%c,%c,%c,%c,%c,%#x)",
		 ipc.request.i, l4util_idstr(ipc.sender),
		 ipc.request.s.wait?'w':'-',
		 ipc.request.s.consume?'c':'-',
		 ipc.request.s.mask?'m':'-',
		 ipc.request.s.unmask?'u':'-',
		 ipc.request.s.again?'a':'-',
		 ipc.request.s.param);
	    
	    ret=handle_user_request(num, &ipc);
	    if(ret==RET_HANDLE) continue;
	    if(ret==RET_WAIT){
		LOGd(OMEGA0_DEBUG_IRQ_THREADS, "sending no reply, waiting");
		error = l4_ipc_wait(&ipc.sender, L4_IPC_SHORT_MSG,
				    &ipc.d0, &ipc.request.i,
				    L4_IPC_NEVER, &result);
	    } else {
		LOGd(OMEGA0_DEBUG_IRQ_THREADS,
		     "sending ipc to "l4util_idfmt" and wait\n\n\n",
		     l4util_idstr(ipc.sender));
		
		error = l4_ipc_reply_and_wait(ipc.sender, L4_IPC_SHORT_MSG,
					      ret,
					     OMEGA0_DEBUG_MEASUREMENT_SENDTIME?
					      (unsigned)(l4_rdtsc()):0,
					      &ipc.sender, L4_IPC_SHORT_MSG,
					      &ipc.d0, &ipc.request.i,
					      L4_IPC_SEND_TIMEOUT_0,
					      &result);
	    }
	}	// ipc from client
    }	// while(1) - outer request loop
}

/*!\brief Initialize the local irq acceptor threads.
 *
 * This function will attach to all irqs possible and initialize the
 * corresponding structures. At the standard-pc-x86 architecture, we
 * have irqs 0 to 15. Try to attach to all of them. This is done by
 * creating 16 threads, one per irq. Threads start at lthread+1.
 *
 * create_threads_sync() is external to allow overloading in other pkgs.
 */
int attach_irqs(void){
    int error, i;
    char buf[140], *p=buf;

    if((error=create_threads_sync())) return error;

    buf[0]=0;
    p+=sprintf(p, "Available IRQs=[ ");
    for(i=0;i<IRQ_NUMS;i++){
	if(irqs[i].available) p+=sprintf(p, "%x ", i);
	else p+=sprintf(p, "<!%x> ", i);
    }
    p+=sprintf(p, "]\n");
    LOG_printf(buf);
    
    return 0;
}
