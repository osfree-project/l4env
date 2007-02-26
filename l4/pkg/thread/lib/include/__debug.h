/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__debug.h
 * \brief  Debug config.
 *
 * \date   08/30/2000
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
#ifndef _THREAD___DEBUG_H
#define _THREAD___DEBUG_H

#ifdef DEBUG

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

#endif /* DEBUG */

#endif /* !_THREAD___DEBUG_H */
