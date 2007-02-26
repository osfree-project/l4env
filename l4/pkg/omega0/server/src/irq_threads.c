#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/rmgr/librmgr.h>
#include <l4/log/l4log.h>
#include <stdlib.h>
#include <stdio.h>
#include <l4/omega0/client.h>
#include <l4/util/rdtsc.h>
#include <l4/util/spin.h>
#include "pic.h"
#include "globals.h"
#include "irq_threads.h"
#include "create_threads.h"	/* krishna: separate thread creation */
#include "config.h"

static void attach_irq(int);



/* signal a single user thread. Assume, the client is waiting.
 *
 * Return value: 0 - the client is signalled, error otherwise.
 */
static void signal_single_user_thread(int irq, client_chain *c){
  int error;
  l4_msgdope_t result;

#ifdef OMEGA0_DEBUG_IRQ_THREADS
  LOGl("Got irq %#x, Wakeup thread %#t", irq,  c->client);
#endif

  // set the counter to the current one.
  c->last_irq_num= irqs[irq].counter;
  error = l4_i386_ipc_send(c->client, L4_IPC_SHORT_MSG, irq+1,
                           #ifdef OMEGA0_DEBUG_MEASUREMENT_SENDTIME
                              (unsigned)(l4_rdtsc().ll),
                           #else
                              0,
                           #endif
                           L4_IPC_TIMEOUT(0,1,0,1,0,0), &result);
  
  // if the ipc was successful, mark the irq as 'in service'
  if(!error){
#ifdef OMEGA0_STRATEGY_AUTO_CONSUME
      /* increment the number of waiting clients. We need this for the
	 automatic consume operation. */
      irqs[irq].clients_waiting++;
#endif
      c->in_service = 1;
  }
  else LOGl("error %#x sending IPC to %#t", error, c->client);
  c->waiting = 0;
}

/* signal all user threads waiting on this irq.
   attention: this does not allow multiple lines per irq up to now,
              because this requires multiple threads per client at the
              server. */
static void signal_user_threads(int irq){
  client_chain *c;
  wq_lock_queue_elem wqe;

  aquire_mutex(&irqs[irq].mutex, &wqe);

  irqs[irq].counter++;
#ifdef OMEGA0_STRATEGY_AUTO_CONSUME
  irqs[irq].clients_waiting=0;
#endif
  
  for(c=irqs[irq].clients;c;c=c->next){
    if(c->waiting){
	signal_single_user_thread(irq,c);
    } else {
      //LOGl("client %#t not waiting", c->client);
    }
  }
  
  release_mutex(&irqs[irq].mutex, &wqe);
}

/* This function is used to actually consume an irq. 

   It checkis if the  IRQ is in service, and if so, it consumes it
   according to the current policy. After that, the in-service flag is
   reset.

   Returns: 0 if the IRQ was in service.
*/
static inline int irq_consume(int irq){
    // irq in service?
    if(!irqs[irq].in_service) return -1;

#ifdef OMEGA0_STRATEGY_IMPLICIT_MASK
    /* it is acknowledged already, but implicitely masked */
    irq_unmask(irq);
#else
    irq_ack(irq);
#endif
    irqs[irq].in_service = 0;
    return 0;
}


#ifdef OMEGA0_STRATEGY_AUTO_CONSUME
/* This function checks if the given client is a candidate for
   auto-consuming an irq. This is the case, if:
   - the client is marked as in service (got a notification)
   - the clients's occurence-number matches that of the IRQ
   - the IRQ is in service
   - this client was the last (i.e., the number if waiting clients = 1)

   It may be that some of the conditions follow from others.

   Pre: The irq-mutex must be hold outside.

   Params: the IRQ, the client
*/
void check_auto_consume(int irq, client_chain *c){

    if(c->in_service && c->last_irq_num == irqs[irq].counter 
       && irqs[irq].in_service && --irqs[irq].clients_waiting==0){

#ifdef OMEGA0_DEBUG_IRQ_THREADS
	  LOGl("Automatically consuming irq %#x", irq);
#endif
	  if(irq_consume(irq)){
	      LOGl("An error occured during automatically consume operation");
	  }
    } else {
#ifdef OMEGA0_DEBUG_IRQ_THREADS
	  LOGl("No automatic-consume of irq %#x, clients_waiting %d now",
	       irq, irqs[irq].clients_waiting);
#endif
    }
}
#endif

/* This function handles clients requests. Following actions are defined:
   wait		- wait until next irq
   mask		- mask the (unmasked) irq
   unmask	- unmask the (masked) irq
   consume	- previously occured irq consumed, may occur again

   If an wait operation occures while the in_service flag is set, the
   client cannot handle the irq. Do nothing. At least one client should
   send the consume action.

   If the OMEGA0_STRATEGY_AUTO_CONSUME option is set, a consume operation is
   automatically done if all clients are waiting an no client consumed
   the irq explicitely (i.e., sent a consume action).
   
   Requests from unregistered clients are ignored silently.
   
   return value: 0   - no additional action needed,
                 !=0 - return an ipc to the client, return value
                       >0 - ok, <0 - error
*/
static int handle_user_request(int irq, l4_threadid_t client,
                                omega0_request_t request){
  client_chain *c;
  int ret;
  wq_lock_queue_elem wqe;

  aquire_mutex(&irqs[irq].mutex, &wqe);
  
  // find the corresponding client structure
  for(c=irqs[irq].clients;c;c=c->next) if(l4_thread_equal(client, c->client)){
  
    // does the client want to unmask the irq and has it masked?
    if(request.s.unmask){
      // wants the client to unmask the correct irq? (currently: only <irq>)
      if(request.s.param!=irq+1) { ret = -1; goto back;}

      //LOGl("c->masked = %d, irqs[irq].masked=%d", c->masked, irqs[irq].masked);
      // is it unmasked?
      if(! c->masked) { ret = -2; goto back;}
      
      // then the irq is masked
      // is it masked exactly once?
      if(--irqs[irq].masked==0) irq_unmask(irq);
      c->masked = 0;
    }
    
    // did the client consume the irq?
    if(request.s.consume){
      // is it not in service?
      if(!c->in_service) { ret = -3; goto back;}
      
      if(irq_consume(irq)){ ret = -4; goto back;}
      
      c->in_service = 0;
    }
    
    // does the client want an irq on next occurence of irq?
    if(request.s.wait){

#ifdef OMEGA0_STRATEGY_AUTO_CONSUME
      check_auto_consume(irq, c);
#endif

      // if in_service flag was set, the driver is not shure it 
      // can handle the irq
      c->in_service = 0;
      c->waiting = 1;

      // IRQ not consumed and not signalled to this thread ?
      // This only happens in non-auto-consume mode.
      if(irqs[irq].in_service && irqs[irq].counter != c->last_irq_num){
        //LOGl("signalling client which is already waiting\n");
        signal_single_user_thread(irq, c);
      }
      
      ret = 0;		// no ipc back
    } else {
      ret = 1;		// ipc back
    }

    // does the client want to mask the irq?
    if(request.s.mask){
      // wants the client to mask the correct irq? (currently: only <irq>)
      if(request.s.param!=irq+1) { ret = -5; goto back;}
    
      // is it masked already?
      if(c->masked) { ret = -6; goto back;}
      
      // is the irq unmasked now?
      if(irqs[irq].masked==0) irq_mask(irq);
      irqs[irq].masked++;
      c->masked = 1;
    }
    
    goto back;	// break the loop
  }
  
  // client not found in the list, ignore the ipc
  LOGl("Client %t not found in the list of attached clients, ignoring request",
       client);
  ret = 0;
  
  back:
  release_mutex(&irqs[irq].mutex, &wqe);
  
  #ifdef OMEGA0_DEBUG_IRQ_THREADS
  LOG("returning %d, forcing %s IPC back", ret, ret?"1":"no");
  #endif
  return ret;
}

/* attach to a specified irq. If the irq is available, its available flag
   is set to 1, otherwise it is cleared. */
static void attach_irq(int num){
  int error;
  l4_msgdope_t result;
  int dummy;
  l4_threadid_t irq_th;
  
  /* the following is very strange. I have no idea why this is necessary,
     but without the sleep it would not work. */
  
  //LOGl("calling rmgr_get_irq(%x)", num);
  if(rmgr_get_irq(num) != 0){
    #ifdef OMEGA0_DEBUG_IRQ_THREADS
      LOGl("error getting irq %#x from rmgr", num);
    #endif
    irqs[num].available = 0;
    return;
  }
  
  irq_th.lh.low = num + 1;
  irq_th.lh.high = 0;
  
  error = l4_i386_ipc_receive(irq_th, 0, &dummy, &dummy,
                              L4_IPC_TIMEOUT(0,1,0,1,0,0), &result);
  
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

/* this is the handler function for one irq. It has to call attach_irq,
   return an IPC to the management thread and wait for incoming IRQs. These
   irqs must be handled in a reasonable way. */
void irq_handler(int num){
  int error;
  l4_msgdope_t result;
  l4_threadid_t mainthread = l4_myself(), sender;
  l4_umword_t dw0, dw1;

/* krishna: setup lthread info */
  irqs[num].lthread = mainthread.id.lthread;

  mainthread.id.lthread = MANAGEMENT_THREAD;
  attach_irq(num);
#ifdef OMEGA0_DEBUG_IRQ_THREADS
  if(irqs[num].available){
    LOGl("irq %#02x available", num);
  } else {
    LOGl("irq %#02x not available", num);
  }
#endif
  
  do{
    error = l4_i386_ipc_send(mainthread, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_NEVER,
                             &result);
    if(error){
      LOGL("error sending birth-ipc to %#t.\n", mainthread);
      enter_kdebug("!");
    }
  }while(error);


  /* wait for ipcs now */

  error = l4_i386_ipc_wait(&sender, L4_IPC_SHORT_MSG, &dw0, &dw1,
                           L4_IPC_NEVER, &result);
  while(1){

    while(error){
      LOGl("IRQ %d: ipc error %#x", num, error);
      enter_kdebug(".");
      error = l4_i386_ipc_wait(&sender, L4_IPC_SHORT_MSG, &dw0, &dw1,
                               L4_IPC_NEVER, &result);
      enter_kdebug("got irq");
    }
    
    if(sender.lh.high==0 && sender.lh.low<=IRQ_NUMS){
      /* irq from kernel */
      
#ifdef OMEGA0_DEBUG_IRQ_THREADS
      l4_spin_text(0, num, "IRQ: ");
#endif
      
      irqs[num].in_service = 1;
      if(num == 7) {
        //LOGl("got irq %d from %x.%x!", num, sender.lh.high,sender.lh.low);
#ifdef OMEGA0_DEBUG_IRQ_THREADS
        l4_spin_text(0,14,"IRQ 7: ");
#endif
      }
      
      #ifdef OMEGA0_STRATEGY_IMPLICIT_MASK
        /* implicitely mask the irq */
        irq_mask(num);

        #ifdef OMEGA0_DEBUG_PIC
          LOG("irq=%x, ISR=%#x, IRR=%#x", num, 
               pic_isr(num>=8), pic_irr(num>=8));
        #endif

        /* acknowledge the irq at the pic */
        irq_ack(num);

        #ifdef OMEGA0_DEBUG_PIC
          LOG("irq=%x, ISR after ack=%#x, IRR=%#x",
               num, pic_isr(num>=8), pic_irr(num>=8));
        #endif
      #endif
      
      /* signal all waiting threads now */
      signal_user_threads(num);
      
      #ifdef OMEGA0_DEBUG_IRQ_THREADS
        LOG("waiting after IRQ");
      #endif
      error = l4_i386_ipc_wait(&sender, L4_IPC_SHORT_MSG, &dw0, &dw1, 
                               L4_IPC_TIMEOUT(0,3,0,0,0,0), &result);
    } else {
      /* ipc from user thread, must be a request */
      
      if(dw0!=OMEGA0_REQUEST){
        #ifdef OMEGA0_DEBUG_REQUESTS
          LOG("non-Omega0 request from %#t", sender);
        #endif
	error = 1;
	continue;
      }
      #ifdef OMEGA0_DEBUG_REQUESTS
        LOG("request %#x from %#t (%c,%c,%c,%c,%#x)", dw1, sender, 
            ((omega0_request_t)dw1).s.wait?'w':'-',
            ((omega0_request_t)dw1).s.consume?'c':'-',
            ((omega0_request_t)dw1).s.mask?'m':'-',
            ((omega0_request_t)dw1).s.unmask?'u':'-',
            ((omega0_request_t)dw1).s.param);
      #endif
         
      if((dw0=handle_user_request(num, sender, (omega0_request_t)dw1))!=0){
        #ifdef OMEGA0_DEBUG_MEASUREMENT_SENDTIME
          dw1 = (unsigned)(l4_rdtsc().ll);
        #endif
        
        #ifdef OMEGA0_DEBUG_IRQ_THREADS 
	  LOG("sending ipc to %t and wait\n\n\n", sender);
        #endif

        error = l4_i386_ipc_reply_and_wait(sender, L4_IPC_SHORT_MSG, dw0, dw1,
                                       &sender, L4_IPC_SHORT_MSG, &dw0, &dw1,
                                       L4_IPC_TIMEOUT(0,1,0,0,0,0),
				       &result);
      } else {
        #ifdef OMEGA0_DEBUG_IRQ_THREADS
          LOG("sending no reply, waiting");
        #endif
        error = l4_i386_ipc_wait(&sender, L4_IPC_SHORT_MSG, &dw0, &dw1, 
				 L4_IPC_TIMEOUT(0,4,0,0,0,0), &result);
      }
    }	// ipc from client
  }	// while(1) - outer request loop
}

/* initialize the local irq acceptor threads. This will attach to all irqs
   possible and initialize the corresponding structures.
   At the standard-pc-x86 architecture, we have irqs 0 to 15. Try to attach
   to all of them. This is done by creating 16 threads, one per irq. 
   Threads start at lthread=1. */
/* krishna: Now, take care of dynamic lthread numbers */
int attach_irqs(void){
  int error, i;
  char buf[140], *p=buf;

/* krishna: call separated function */
  if((error=create_threads_sync())) return error;

  buf[0]=0;
  p+=sprintf(p, "available irqs=[ ");
  for(i=0;i<IRQ_NUMS;i++){
    if(irqs[i].available) p+=sprintf(p, "%x ", i);
    else p+=sprintf(p, "<!%x> ", i);
  }
  p+=sprintf(p, "]");
  LOGl(buf);
  
  return 0;
}
