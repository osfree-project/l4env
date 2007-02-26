#ifndef __OMEGA0_SERVER_GLOBALS_H
#define __OMEGA0_SERVER_GLOBALS_H

#include <l4/sys/types.h>
#include "config.h"
#include "lock.h"

#define IRQ_NUMS 16	// 16 at standard-x86-pc architecture 
#define STACKSIZE 4096	// 4KB of stack per irq handler

typedef struct client_chain{
  l4_threadid_t client;
  int		masked;		// this client masked the irq
  int		waiting;	// client is in blocking state
  int		in_service;	// this irq is in service by this client
  unsigned      last_irq_num;	// last irq num handled by this client
  struct client_chain*next;
} client_chain;

typedef wq_lock_queue_base mutex_t;

typedef struct{
  int		available;		// available to omega0
  int		shared;			// shared or not
  int		masked;			// counter for masks, no mask if 0
  int		in_service;		// 1 if not consumed yet
#ifdef OMEGA0_STRATEGY_AUTO_CONSUME
  int 		clients_waiting;	// # of clients waiting for this irq
  int		clients_registered;	// # of registered clients for this irq
#endif
  client_chain	*clients;		// the clients
  mutex_t	mutex;			// mutex for client-chain
  unsigned      counter;		// last irq nr occured
/* krishna: use dynamic stack allocation - please */
  l4_umword_t *	stack;			// stack for irq handler
/* krishna: lthread (= handle) information */
  unsigned	lthread;		// l4_myself.id.lthread
} irq_description_t;

extern irq_description_t irqs[IRQ_NUMS];

static inline void aquire_mutex(mutex_t *wq, wq_lock_queue_elem *elem){
    wq_lock_lock(wq, elem);
}

static inline void release_mutex(mutex_t *wq, wq_lock_queue_elem *elem){
    wq_lock_unlock(wq, elem);
}

//void aquire_mutex(mutex_t*);
//void release_mutex(mutex_t*);

#endif
