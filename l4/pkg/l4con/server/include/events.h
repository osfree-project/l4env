/*****************************************************************************/
/**
 * \file   con/server/include/events.h
 * \brief  Event handling
 *
 * \date   01/03/2004
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef CON_EVENTS_H
#define CON_EVENTS_H

/*****************************************************************************
 *** config
 *****************************************************************************/

/**
 * event notification thread priority
 */
#define _CON_EVENT_THREAD_PRIORITY	(14)

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* init event handling */
void
init_events(void);

#endif /* CON_EVENTS_H */
