/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/include/__debug.h
 * \brief  Debug config
 *
 * \date   11/21/2001
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
#ifndef _DM_GENERIC___DEBUG_H
#define _DM_GENERIC___DEBUG_H

#ifdef DEBUG

/* enable debug assertions */
#define DEBUG_ASSERTIONS           1

/* be more verbose on internal errors */
#define DEBUG_ERRORS               1

/* 1 enables debug output, 0 disables */
#define DEBUG_PAGE_ALLOC           0
#define DEBUG_HASH                 0
#define DEBUG_DS_ID                0

#endif /* DEBUG */

#endif /* !_DM_GENERIC___DEBUG_H */
