/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__debug.h
 * \brief  Debug config.
 *
 * \date   08/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___DEBUG_H
#define _THREAD___DEBUG_H

/* enable debug assertions */
#define DEBUG_ASSERTIONS        1

/* be more verbose on errors */
#define DEBUG_ERRORS            1

/* 1 enables debug output, 0 disables */
#define DEBUG_CONFIG            0
#define DEBUG_MEM_ALLOC         0
#define DEBUG_MEM_FREE          0
#define DEBUG_STACK_INIT        0
#define DEBUG_STACK_ALLOC       0
#define DEBUG_TCB_INIT          0
#define DEBUG_TCB_ALLOC         0
#define DEBUG_PRIO_INIT         0
#define DEBUG_CREATE            0
#define DEBUG_STARTUP           0
#define DEBUG_SLEEP             0
#define DEBUG_SANITY		0

#endif /* !_THREAD___DEBUG_H */
