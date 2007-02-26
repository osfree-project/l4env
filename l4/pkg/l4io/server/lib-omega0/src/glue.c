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

/** Startup for Omega0 server thread.
 * \ingroup grp_o0 */
static void server_startup(void *fake)
{
  /* register at names */
  if (!names_register(OMEAG0_SERVER_NAME))
    {
      Panic("[OMEGA0lib] can't register at names");
    }

  if (l4thread_started(NULL))
    Panic("[OMEGA0lib] startup notification failed");

  /* call original server code */
  server();

  while (1)
    {
      Panic("[OMEGA0lib] server() returned");
    }
}

/** Startup for Omega0 IRQ handlers.
 * \ingroup grp_o0 */
static void irq_handler_startup(void *fake_nr)
{
  LOGd(DO_DEBUG, "omega0_irq_thread[%d] "l4util_idfmt" running.",
       (int)fake_nr, l4util_idstr(l4thread_l4_id(l4thread_myself())));

  irq_handler((int) fake_nr);

  while (1)
    {
      Panic("[OMEGA0lib] irq_handler() returned");
    }
}

/** Provide separated thread creation to Omega0 sources.
 * \ingroup grp_o0 */
int create_threads_sync(void)
{
  int i, error;
  l4thread_t irq_tid;
  l4_umword_t dummy;
  l4_msgdope_t result;
  char name[16];

  /* server thread */
  MANAGEMENT_THREAD = l4thread_l4_id(l4thread_myself()).id.lthread;

  /* IRQ threads */
  for (i = 0; i < IRQ_NUMS; i++)
    {
      snprintf(name, sizeof(name), ".irq%.2X", i);
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
  l4thread_t dummy = L4THREAD_INVALID_ID;
  int noparam;

  use_special_fully_nested_mode = use_spec;
  LOG("Using %s fully nested PIC mode",
       use_special_fully_nested_mode ? "special" : "");

  /* create omega0 irq threads */
  attach_irqs();

  /* create omega0 server thread */
  dummy = l4thread_create_long(L4THREAD_INVALID_ID,
                               server_startup,
                               ".irq-mgr",
                               L4THREAD_INVALID_SP,
                               L4THREAD_DEFAULT_SIZE,
                               L4THREAD_DEFAULT_PRIO,
                               (void *) &noparam,
                               L4THREAD_CREATE_SYNC);

  if (dummy < 0)
    return dummy;

  LOGd(DO_DEBUG, "omega0_server_thread "l4util_idfmt" running.",
       l4util_idstr(l4thread_l4_id(dummy)));

  return 0;
}
/** @} */
