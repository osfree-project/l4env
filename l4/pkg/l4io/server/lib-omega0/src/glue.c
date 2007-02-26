/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/lib-omega0/src/glue.c
 *
 * \brief	L4Env l4io OMEGA0lib Server Glue Code
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4 */
#include <l4/names/libnames.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>

/* OSKit */
#include <stdio.h>

/* local */
#include "internal.h"
#include "omega0lib.h"

/* omega0 internals */
#include <omega0_proto.h>
#include <server.h>
#include <irq_threads.h>
#include <create_threads.h>

/*****************************************************************************/
/*
 * global vars
 */
/*****************************************************************************/
unsigned MANAGEMENT_THREAD;	/**< omega0 management thread 
				 * \ingroup grp_o0 */

/*****************************************************************************/
/** Startup for Omega0 server thread.
 * \ingroup grp_o0
 */
/*****************************************************************************/
static void server_startup(void *fake)
{
  /* register at names */
  if (!names_register(OMEAG0_SERVER_NAME))
    {
      PANIC("[OMEGA0lib] can't register at names");
    }

  if (l4thread_started(NULL))
    PANIC("[OMEGA0lib] startup notification failed");

  /* call original server code */
  server();

  while (1)
    {
      PANIC("[OMEGA0lib] server() returned");
    }
}

/*****************************************************************************/
/** Startup for Omega0 IRQ handlers.
 * \ingroup grp_o0
 */
/*****************************************************************************/
static void irq_handler_startup(void *fake_nr)
{
  DMSG("omega0_irq_thread[%d] "IdFmt" running.\n",
       (int)fake_nr, IdStr(l4thread_l4_id(l4thread_myself())));

  irq_handler((int) fake_nr);

  while (1)
    {
      PANIC("[OMEGA0lib] irq_handler() returned");
    }
}

/*****************************************************************************/
/** Provide separated thread creation to Omega0 sources.
 * \ingroup grp_o0
 */
/*****************************************************************************/
int create_threads_sync(void)
{
  int i, error;
  l4thread_t irq_tid;
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* server thread */
  MANAGEMENT_THREAD = l4thread_l4_id(l4thread_myself()).id.lthread;

  /* IRQ threads */
  for (i = 0; i < IRQ_NUMS; i++)
    {
      irq_tid = l4thread_create((l4thread_fn_t) irq_handler_startup,
				(void *) i, L4THREAD_CREATE_ASYNC);

      if (irq_tid < 0)
	PANIC("[OMEGA0lib] thread creation failed");

      /* notification */
      error = l4_i386_ipc_receive(l4thread_l4_id(irq_tid),
				  L4_IPC_SHORT_MSG, &dummy, &dummy,
				  L4_IPC_NEVER, &result);
      if (error)
	return -1;
    }
  return 0;
}

/*****************************************************************************/
/**
 * \name OMEGA0lib interface
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** OMEGA0lib initialization.
 * \ingroup grp_o0
 */
/*****************************************************************************/
int OMEGA0_init()
{
  l4thread_t dummy = L4THREAD_INVALID_ID;
  int noparam;

  /* create omega0 irq threads */
  attach_irqs();

  /* create omega0 server thread */
  dummy = l4thread_create((l4thread_fn_t) server_startup,
			  (void *) &noparam, L4THREAD_CREATE_SYNC);

  if (dummy < 0)
    return dummy;

  DMSG("omega0_server_thread "IdFmt" running.\n", IdStr(l4thread_l4_id(dummy)));

  return 0;
}

/** @} */
