/*****************************************************************************/
/**
 * \file   loader/server/src/events.h
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
#ifndef LOADER_EVENTS_H
#define LOADER_EVENTS_H

/*****************************************************************************
 *** config
 *****************************************************************************/

/**
 * event notification thread no
 */
#define _LOADER_EVENT_THREAD_NO		(5)

/**
 * event notification thread priority
 */
#define _LOADER_EVENT_THREAD_PRIORITY	(13)

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init event handling */
void
init_events(void);

#endif /* LOADER_EVENTS_H */

