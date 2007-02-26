/* $Id$ */
/*****************************************************************************/
/**
 * \file   dmphys/server/include/__events.h
 * \brief  Event handling
 *
 * \date   01/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS___EVENTS_H
#define _DM_PHYS___EVENTS_H

/*****************************************************************************
 *** config
 *****************************************************************************/

/**
 * event notification thread no
 */
#define DMPHYS_EVENT_THREAD_NO          (1)

/**
 * event notification thread priority
 */
#define DMPHYS_EVENT_THREAD_PRIORITY	(12)

/**
 * event thread stack size
 */
#define DMPHYS_EVENT_THRAD_STACK_SIZE   (16 * 1024)

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init event handling */
void
init_events(void);

#endif /* _DM_PHYS___EVENTS_H */

