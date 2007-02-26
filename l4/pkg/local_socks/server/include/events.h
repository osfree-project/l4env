/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/include/events.h
 * \brief  Header for socket server 'events' support.
 *
 * \date   16/01/2006
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOCAL_SOCKS_EVENTS_H
#define __LOCAL_SOCKS_EVENTS_H

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/sys/types.h>

/* ******************************************************************* */

int init_events(void);

#endif /* __LOCAL_SOCKS_EVENTS_H */

