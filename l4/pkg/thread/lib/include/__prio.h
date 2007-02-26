/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__prio.h
 * \brief  Priority handling.
 *
 * \date   01/06/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
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
