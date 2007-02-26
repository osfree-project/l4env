/*****************************************************************************/
/**
 * \file   names/server/include/events.h
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

#ifndef NAMES_EVENTS_H
#define NAMES_EVENTS_H

/*****************************************************************************
 *** config
 *****************************************************************************/

/**
 * event notification thread no
 */
#define NAMES_EVENT_THREAD_NO		(1)

/**
 * event notification thread priority
 */
#define NAMES_EVENT_THREAD_PRIORITY	(15)

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init event handling */
void
init_events(void);

#endif /* NAMES_EVENTS_H */

