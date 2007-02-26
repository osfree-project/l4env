/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/lib-omega0/src/glue.c
 * \brief  L4Env l4io OMEGA0lib Server Glue Code
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/names/libnames.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>
#include <l4/util/macros.h>

/* OSKit */
#include <stdio.h>

/* local */
#include "omega0lib.h"

/* omega0 internals */
#include <omega0_proto.h>
#include <server.h>
#include <irq_threads.h>
#include <create_threads.h>
#include <pic.h>

/* do debugging */
#define DO_DEBUG  0

/*
 * global vars
 */
unsigned MANAGEMENT_THREAD;  /**< omega0 management thread
                              * \ingroup grp_o0 */

static l4_threadid_t omega0_service_id = L4_INVALID_ID;

/** Startup for Omega0 server thread.
 * \ingroup grp_o0 */
static void server_startup(void *fake)
{
  /* register at names */
  if (!names_register(OMEAG0_SERVER_NAME))
    Panic("[OMEGA0lib] can't register at names");

  if (l4thread_started(NULL))
    Panic("[OMEGA0lib] startup notification failed");

  /* call original server code */
  server();

  while (1)
    Panic("[OMEGA0lib] server() returned");
}

/** Startup for Omega0 IRQ handlers.
 * \ingroup grp_o0 */
static void irq_handler_startup(void *fake_nr)
{
  LOGd(DO_DEBUG, "omega0_irq_thread[%ld] "l4util_idfmt" running.",
       (long int)fake_nr, l4util_idstr(l4thread_l4_id(l4thread_myself())));

  irq_handler((int long) fake_nr);

  while (1)
    Panic("[OMEGA0lib] irq_handler() returned");
}

/** Provide separated thread creation to Omega0 sources.
 * \ingroup grp_o0 */
int create_threads_sync(void)
{
  long int i, error;
  l4thread_t irq_tid;
  l4_umword_t dummy;
  l4_msgdope_t result;
  char name[16];

  /* server thread */
  MANAGEMENT_THREAD = l4thread_l4_id(l4thread_myself()).id.lthread;

  /* IRQ threads */
  for (i = 0; i < IRQ_NUMS; i++)
    {
      snprintf(name, sizeof(name), ".irq%.2lX", i);
      irq_tid = l4thread_create_long(L4THREAD_INVALID_ID,
                                     irq_handler_startup,
                                     name,
                                     L4THREAD_INVALID_SP,
                                     L4THREAD_DEFAULT_SIZE,
                                     L4THREAD_DEFAULT_PRIO,
                                     (void *) i,
                                     L4THREAD_CREATE_ASYNC);

      if (irq_tid < 0)
        Panic("[OMEGA0lib] thread creation failed");

      /* hande shake with original Omega0 implementation */
      error = l4_ipc_receive(l4thread_l4_id(irq_tid),
                             L4_IPC_SHORT_MSG, &dummy, &dummy,
                             L4_IPC_NEVER, &result);
      if (error)
        return -1;
    }
  return 0;
}

/** \name OMEGA0lib interface
 * @{ */

/** OMEGA0lib initialization.
 * \ingroup grp_o0 */
int OMEGA0_init(int use_spec)
{
  l4thread_t thread = L4THREAD_INVALID_ID;
  int noparam;

  use_special_fully_nested_mode = use_spec;
  LOG("Using %s fully nested PIC mode",
       use_special_fully_nested_mode ? "special" : "");

  /* create omega0 irq threads */
  attach_irqs();

  /* create omega0 server thread */
  thread = l4thread_create_long(L4THREAD_INVALID_ID,
                               server_startup,
                               ".irq-mgr",
                               L4THREAD_INVALID_SP,
                               L4THREAD_DEFAULT_SIZE,
                               L4THREAD_DEFAULT_PRIO,
                               (void *) &noparam,
                               L4THREAD_CREATE_SYNC);

  if (thread < 0)
    return thread;

  omega0_service_id = l4thread_l4_id(thread);

  LOGd(DO_DEBUG, "omega0_server_thread "l4util_idfmt" running.",
       l4util_idstr(l4thread_l4_id(thread)));

  return 0;
}

/** Free all ressources of a specific client.
 * \ingroup grp_o0 */
void OMEGA0_free_ressources(l4_threadid_t client)
{
  int res;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;

  if (l4_is_invalid_id(omega0_service_id))
    return;

  res = l4_ipc_call(omega0_service_id,
		   L4_IPC_SHORT_MSG, OMEGA0_DETACH_ALL, client.id.task,
		   L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_NEVER, &result);
}

/** @} */
