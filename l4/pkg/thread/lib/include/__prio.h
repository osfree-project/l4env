/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__prio.h
 * \brief  Priority handling.
 *
 * \date   01/06/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___PRIO_H
#define _THREAD___PRIO_H

/* lib includes */
#include <l4/thread/thread.h>
#include "__tcb.h"

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

int
l4th_prio_init(void);

l4_prio_t
l4th_get_prio(l4th_tcb_t * tcb);

int
l4th_set_prio(l4th_tcb_t * tcb, l4_prio_t prio);


#endif /* !_THREAD___PRIO_H */
