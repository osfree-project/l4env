/**
 * \file   omega0/server/src/config.h
 * \brief  Configuration
 *
 * \date   2007-04-27
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __OMEGA0_SERVER_CONFIG_H
#define __OMEGA0_SERVER_CONFIG_H

// set to 1 if you want verbose output in irq_threads.c
#define OMEGA0_DEBUG_IRQ_THREADS 0

// set to 1 if you want logging of user requests
#define OMEGA0_DEBUG_REQUESTS 0

// set to 1 if you want verbose output of pic-related actions
#define OMEGA0_DEBUG_PIC 0

// se to 1 if you want the lowest 32bit of TSC in dw1 when sending an
// IPC to clients
#define OMEGA0_DEBUG_MEASUREMENT_SENDTIME 0

// set to 1 if you want verbose startup
#define OMEGA0_DEBUG_STARTUP 0

/* set to 1 if you want the server to enter-kdebug on fairly hard
 * errors (like memory shortage or other unexpected things). */
#define ENTER_KDEBUG_ON_ERRORS 1

/* set to 1 to print error messages about invalid requests or
 * failed wakeups of client */
#define OMEGA0_DEBUG_WARNING 0

/* set to 1 if you want the server to automatically consume shared
 * irqs if all clients wait for the irq. */
#define OMEGA0_STRATEGY_AUTO_CONSUME 1

#endif
