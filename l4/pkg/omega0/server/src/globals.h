/**
 * \file   omega0/server/src/globals.h
 * \brief  Omega0 server global declarations
 *
 * \date   2007-04-27
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __OMEGA0_SERVER_GLOBALS_H
#define __OMEGA0_SERVER_GLOBALS_H

#include <l4/sys/types.h>
#include <l4/util/lock_wq.h>
#include <l4/omega0/client.h>
#include "config.h"

#define IRQ_NUMS 18	// 16 at standard-x86-pc architecture 
#define STACKSIZE 4096	// 4KB of stack per irq handler

typedef struct client_chain{
  l4_threadid_t client;
  int		masked;		// this client masked the irq
  int		waiting;	// client is in blocking state
  int		in_service;	// this irq is in service by this client
  unsigned      last_irq_num;	// last irq num handled by this client
  omega0_request_t last_request;/* last request of the client, successful
				 * operations deleted */
  struct client_chain*next;
} client_chain;

typedef l4util_wq_lock_queue_base_t mutex_t;

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

static inline void aquire_mutex(mutex_t *wq,
				l4util_wq_lock_queue_elem_t *elem){
    l4util_wq_lock_lock(wq, elem);
}

static inline void release_mutex(mutex_t *wq,
				 l4util_wq_lock_queue_elem_t *elem){
    l4util_wq_lock_unlock(wq, elem);
}

#endif
