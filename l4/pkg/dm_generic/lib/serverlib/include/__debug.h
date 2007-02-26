/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/include/__debug.h
 * \brief  Debug config
 *
 * \date   11/21/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_GENERIC___DEBUG_H
#define _DM_GENERIC___DEBUG_H

/* enable debug assertions */
#define DEBUG_ASSERTIONS           1

/* be more verbose on internal errors */
#define DEBUG_ERRORS               1

/* 1 enables debug output, 0 disables */
#define DEBUG_PAGE_ALLOC           0
#define DEBUG_HASH                 0
#define DEBUG_DS_ID                0

#endif /* !_DM_GENERIC___DEBUG_H */
