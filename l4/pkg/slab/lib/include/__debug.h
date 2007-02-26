/* $Id$ */
/*****************************************************************************/
/**
 * \file   slab/lib/include/__debug.h
 * \brief  Debug config.
 *
 * \date   2006-12-18
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _SLAB___DEBUG_H
#define _SLAB___DEBUG_H

/* 1 enables debug output, 0 disables */
#define DEBUG_SLAB_INIT         0
#define DEBUG_SLAB_GROW         0
#define DEBUG_SLAB_ASSERT       1

#if DEBUG_SLAB_ASSERT
#define Assert(expr) \
	do {                                                 \
		if (!(expr)) {                                   \
			LOG_printf("ASSERTION FAILED: " #expr "\n"); \
			while (1);                                   \
		}                                                \
	} while (0)
#else
#define Assert(expr) do { } while (0)
#endif

#endif /* !_L4ENV___DEBUG_H */
