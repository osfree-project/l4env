/**
 * \file   omega0/server/src/server.c
 * \brief  IPC server implementation
 *
 * \date   2007-04-27
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/omega0/client.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/log/l4log.h>
#include <stdlib.h>
#include <omega0_proto.h>

#include "globals.h"
#include "server.h"
#include "irq_threads.h"

/*!\brief Attach the client to the given irq.
 *
 * Currently, only one irq per client possible.
 *
 * \retval 0  - success
 * \retval <0 - error,
 * \retval >0 - lthread
 */
static int
attach(l4_threadid_t client, omega0_irqdesc_t desc)
{
  int num;
  client_chain*c;
  l4util_wq_lock_queue_elem_t wqe;  

  desc.s.num--;
  if (desc.s.num > IRQ_NUMS)
    return -1;
  if (!irqs[desc.s.num].available)
    return -1;

  /* check all client-chains for the given id. Because functions in this file
   * are the only ones modifying the client-lists, we do need no locking
   * for scanning. */
  for (num=0;num<IRQ_NUMS; num++)
    {
      for (c = irqs[num].clients;c;c=c->next){
	  if (l4_thread_equal(client, c->client))
	    return -1;
      }
  }
  /* client is not registered. */

  /* does the client want a non-shared irq but the irq is attached already,
     or is the irq attached unshared already? */
  if (irqs[desc.s.num].clients && 
      (!desc.s.shared || !irqs[desc.s.num].shared))
    return -1;

  /* Now register it. Here we need locking. */
  c = malloc(sizeof(client_chain));
  if(!c)
    {
      LOGl("error getting %ld bytes of memory", (unsigned long)sizeof(client_chain));
      if(ENTER_KDEBUG_ON_ERRORS)
	enter_kdebug("!");
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
  irqs[desc.s.num].masked++;
  irqs[desc.s.num].clients_registered++;
  c->masked = 1;
  release_mutex(&irqs[desc.s.num].mutex, &wqe);

  LOGdl(OMEGA0_DEBUG_REQUESTS,
	"client [%x.%x] registered to irq 0x%lx, registered_clients=%d\n",
	client.id.task, client.id.lthread,
	(unsigned long)desc.s.num,
	(unsigned)irqs[desc.s.num].clients_registered);

  return irqs[desc.s.num].lthread;
}


/** detach from a given irq. */
static int
detach(l4_threadid_t client, int irq, int cmp_task)
{
  client_chain *c, *b;
  l4util_wq_lock_queue_elem_t wqe;

  aquire_mutex(&irqs[irq].mutex, &wqe);

  b = NULL;
  for (c=irqs[irq].clients; c; c=c->next)
    {
      if (( cmp_task && l4_tasknum_equal(client, c->client)) ||
	  (!cmp_task && l4_thread_equal(client, c->client)))
	break;
      b = c;
    }
  if (!c)
    {
      release_mutex(&irqs[irq].mutex, &wqe);
      return -1;	// client not found
    }

  if(OMEGA0_STRATEGY_AUTO_CONSUME)
    {
      check_auto_consume(irq, c);
      irqs[irq].clients_registered--;
    }

  if (b)
    b->next = c->next;
  else
    irqs[irq].clients = c->next;
  release_mutex(&irqs[irq].mutex, &wqe);

  free(c);
  return 0;
}

/** detach from a given client. */
static int
detach_all(int task)
{
  int irq;
  l4_threadid_t client;

  client.id.task = task;

  for (irq=0; irq<IRQ_NUMS; irq++)
    detach(client, irq, 1);

  return 0;
}

/** Pass IRQ to other thread.
 *
 * \bug It may happen that the detach is successful, but not so the
 *      attach. at the result, the irq is free (!!!!) */
static int
pass(l4_threadid_t client, omega0_irqdesc_t desc, l4_threadid_t new_client)
{
  if (detach(client, desc.s.num-1, 0)==0) 
    return attach(new_client, desc);
  else
    return -1;
}

/** Iterator: First handle */
static int
first(void)
{
  int i;
  
  for (i=0;i<IRQ_NUMS;i++)
    if (irqs[i].available)
      return i+1;

  return 0;
}

/** Iterator: Next handle */
static int
next(int irq)
{
  for (;irq<IRQ_NUMS;irq++)
    if (irqs[irq].available)
      return irq+1;
  return 0;
}

/*!\brief The server itself.
 *
 * All initialization stuff like creating the receive- threads which
 * are attached to their irqs is already done. */
void
server(void)
{
  l4_threadid_t client;
  extern l4_threadid_t events_thread_id;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  int error, ret;
  struct
    {
      l4_fpage_t      rcv_fpage;
      l4_msgdope_t    size;
      l4_msgdope_t    snd;
      l4_umword_t     dw0, dw1;
      l4_threadid_t   thread;
    } msg;

  msg.size = L4_IPC_DOPE(4,0);
  error = l4_ipc_wait(&client, &msg, &dw0, &dw1, L4_IPC_NEVER, &result);
  while (1)
    {
      if (error)
	{
	  LOGl("IPC error %#x", error);
	  msg.size = L4_IPC_DOPE(4,0);
	  error = l4_ipc_wait(&client, &msg, &dw0, &dw1,
			      L4_IPC_NEVER, &result);
	  continue;
	}
    
      switch(dw0)
	{
	case OMEGA0_ATTACH:
	  ret = attach(client, (omega0_irqdesc_t)dw1);
	  break;
	case OMEGA0_DETACH:
	  ret = detach(client, ((omega0_irqdesc_t)dw1).s.num-1, 0);
	  break;
	case OMEGA0_DETACH_ALL:
	  if (l4_thread_equal(client, events_thread_id))
	    ret = detach_all(dw1);
	  else
	    {
	      LOGl("Ignoring detach_all request from "l4util_idfmt,
	 	  l4util_idstr(client));
	      ret = -1;
	    }
	  break;
	case OMEGA0_PASS:
	  ret = pass(client, (omega0_irqdesc_t)dw1, msg.thread);
	  break;
	case OMEGA0_FIRST:
	  ret = first();
	  break;
	case OMEGA0_NEXT:
	  ret = next(dw1);
	  break;
	default:
	  ret = -1;
	  break;
	}
      msg.size = L4_IPC_DOPE(4,0);
      error = l4_ipc_reply_and_wait(client, L4_IPC_SHORT_MSG, ret, 0,
				    &client, &msg, &dw0, &dw1,
			      	    L4_IPC_SEND_TIMEOUT_0, &result);
    }
}
