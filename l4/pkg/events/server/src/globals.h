/*!
 * \file   events/server/src/globals.h
 *
 * \brief  Some macros.
 *
 * \date   19/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "server.h"
#include "l4/events/events.h"

#ifndef __L4EVENTS_GLOBALS
#define __L4EVENTS_GLOBALS


/* runtime debug level */
extern int verbosity;

/* macro for controlling the debug level */
#define DEBUGLVL(LVL) (LVL <= verbosity)

#define false 0
#define true 1

/* value of the timeout: ~10ms */
#define SERVER_TIMEOUT	L4_IPC_TIMEOUT(0,0,156,12,0,0)

/* number of timeouts before releasing the event to the next priority class */
#define TIMEOUT_TICKS 	3

/*!\ingroup priavtelibapi maximum number of channels */
#define MAX_CHANNELS	15

/* maximum number of events */
#define MAX_EVENT_NR	4096

#endif
