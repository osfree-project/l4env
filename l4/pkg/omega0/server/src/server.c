#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/omega0/client.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <stdlib.h>
#include <omega0_proto.h>
#include "pic.h"
#include "globals.h"
#include "server.h"
#include "irq_threads.h"

static int attach(l4_threadid_t client, omega0_irqdesc_t desc);
static int detach(l4_threadid_t client, omega0_irqdesc_t desc);
static int pass(l4_threadid_t client, omega0_irqdesc_t desc,
                l4_threadid_t new_client);
static int first(void);
static int next(int irq);

/* clients wants to attach to given irq. Currently, only one irq per client
   possible.
   
   return value: 0 - success, <0 - error, >0 - lthread */
int attach(l4_threadid_t client, omega0_irqdesc_t desc){
  int num;
  client_chain*c;
  wq_lock_queue_elem wqe;  

  desc.s.num--;
  if(desc.s.num > IRQ_NUMS) return -1;
  if(!irqs[desc.s.num].available) return -1;
  
  /* check all client-chains for the given id. Because functions in this file
     are the only ones modifying the client-lists, we do need no locking
     for scanning. */
  for(num=0;num<IRQ_NUMS; num++){
    for(c = irqs[num].clients;c;c=c->next){
      if(l4_thread_equal(client, c->client)) return -1;
    }
  }
  /* client is not registered. */
  
  /* does the client want a non-shared irq but the irq is attached already,
     or is the irq attached unshared already? */
  if( irqs[desc.s.num].clients && 
      (!desc.s.shared || !irqs[desc.s.num].shared) )  return -1;
  
  /* Now register it. Here we need locking. */
  
  c = malloc(sizeof(client_chain));
  if(!c){
    LOGl("error getting %d bytes of memory", sizeof(client_chain));
    #ifdef ENTER_KDEBUG_ON_ERRORS
      enter_kdebug("!");
    #endif
    return -1;
  }
  
  c->client = client;
  c->masked = 0;
  c->waiting = 0;
  c->in_service = 0;
  aquire_mutex(&irqs[desc.s.num].mutex, &wqe);
  c->last_irq_num = irqs[desc.s.num].counter;
  c->next = irqs[desc.s.num].clients;
  irqs[desc.s.num].clients = c;
  irqs[desc.s.num].shared = desc.s.shared;
  irq_mask(desc.s.num);
  irqs[desc.s.num].masked++;
  irqs[desc.s.num].clients_registered++;
  c->masked = 1;
  release_mutex(&irqs[desc.s.num].mutex, &wqe);

#ifdef OMEGA0_DEBUG_REQUESTS
  LOGl("client [%x.%x] registered to irq %#x, registered_clients=%d",
       client.id.task, client.id.lthread,
       desc.s.num,
       irqs[desc.s.num].clients_registered);
#endif

/* krishna: Now, return lthread */
  return irqs[desc.s.num].lthread;
}


/* detach from a given irq. */
int detach(l4_threadid_t client, omega0_irqdesc_t desc){
  client_chain *c, *b;
  wq_lock_queue_elem wqe;

  desc.s.num--;

  aquire_mutex(&irqs[desc.s.num].mutex, &wqe);

  b = NULL;
  for(c = irqs[desc.s.num].clients;c;c=c->next){
    if(l4_thread_equal(client, c->client)) break;
    b = c;
  }
  if(!c){
      release_mutex(&irqs[desc.s.num].mutex, &wqe);
      return -1;	// client not found
  }

#ifdef OMEGA0_STRATEGY_AUTO_CONSUME
  check_auto_consume(desc.s.num, c);
  irqs[desc.s.num].clients_registered--;
#endif

  // did the client mask the irq? It must first unmask it. 
  // check removed, still unsolved.
  // UNSOLVED_MASK_DETACH
  //if(c->masked) return -1;
  
  if(b){
    b->next = c->next;
  } else {
    irqs[desc.s.num].clients = c->next;
  }
  release_mutex(&irqs[desc.s.num].mutex, &wqe);

  free(c);
  return 0;
}

/* UNSOLVED_PASS: it may happen that the detach is successful, but not so
   the attach. at the result, the irq is free (!!!!) */
int pass(l4_threadid_t client, omega0_irqdesc_t desc,
         l4_threadid_t new_client){
  if(detach(client, desc)==0) 
    return attach(new_client, desc);
  else return -1;
}

int first(void){
  int i;
  
  for(i=0;i<IRQ_NUMS;i++){
    if(irqs[i].available) return i+1;
  }
  return 0;
}

int next(int irq){

  for(;irq<IRQ_NUMS;irq++){
    if(irqs[irq].available) return irq+1;
  }
  return 0;
}

/* the server itself. All initialization stuff like creating the receive-
   threads which are attached to their irqs is already done. */
void server(void){
  l4_threadid_t client;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  int error, ret;
  struct{
  	l4_fpage_t      rcv_fpage;
	l4_msgdope_t    size;
	l4_msgdope_t    snd;
	l4_threadid_t   new_thread;
	} msg;

  msg.size = L4_IPC_DOPE(4,0);
  error = l4_i386_ipc_wait(&client, &msg, &dw0, &dw1,
                           L4_IPC_NEVER, &result);
  while(1){
    if(error){
      LOGl("IPC error %#x", error);
      msg.size = L4_IPC_DOPE(4,0);
      error = l4_i386_ipc_wait(&client, &msg, &dw0, &dw1,
                               L4_IPC_NEVER, &result);
      continue;
    }
    
    switch(dw0){
      case	OMEGA0_ATTACH:	ret = attach(client, (omega0_irqdesc_t)dw1);
      				break;
      case	OMEGA0_DETACH:	ret = detach(client, (omega0_irqdesc_t)dw1);
      				break;
      case	OMEGA0_PASS:	ret = pass(client, (omega0_irqdesc_t)dw1,
					   msg.new_thread);
      				break;
      case	OMEGA0_FIRST:	ret = first();
      				break;
      case	OMEGA0_NEXT:	ret = next(dw1);
      				break;
      default:	ret = -1;
      		break;
    }
    msg.size = L4_IPC_DOPE(4,0);
    error = l4_i386_ipc_reply_and_wait(client, L4_IPC_SHORT_MSG, ret, 0,
                                       &client, &msg, &dw0, &dw1,
                                       L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
  } // while(1) - outer server loop
}

