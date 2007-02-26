/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/include/__debug.h
 * \brief  Debug config. 
 *
 * \date   05/31/2000 
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
#ifndef _L4RM___DEBUG_H
#define _L4RM___DEBUG_H

#ifdef DEBUG

/* enable debug assertions */
#define DEBUG_ASSERTIONS        1

/* be more verbose on errors */
#define DEBUG_ERRORS            1

/* 1 enables debug output, 0 disables */
#define DEBUG_PAGEFAULT         0 /* be careful: both DEBUG_PAGEFAULT and */
#define DEBUG_REQUEST           0 /* DEBUG_REQUEST might cause deadlocks if
				   * used with the log server */
#define DEBUG_ATTACH            0
#define DEBUG_DETACH            0
#define DEBUG_LOOKUP            0
#define DEBUG_DETACH_UNMAP      0
#define DEBUG_AVLT              0
#define DEBUG_AVLT_ALLOC        0
#define DEBUG_AVLT_INSERT       0
#define DEBUG_AVLT_REMOVE       0
#define DEBUG_AVLT_SHOW         0
#define DEBUG_REGION_INIT       0
#define DEBUG_REGION_INSERT     0
#define DEBUG_REGION_FIND       0
#define DEBUG_REGION_MODIFY     0
#define DEBUG_REGION_RESERVE    0
#define DEBUG_REGION_TREE       0
#define DEBUG_ALLOC_INIT        0
#define DEBUG_ALLOC_PAGE        0

#endif /* DEBUG */

#endif /* !_L4RM___DEBUG_H */
