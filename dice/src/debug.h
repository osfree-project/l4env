/**
 *	\file	dice/src/debug.h
 *	\brief	contains basic macros and definitions for debugging
 *
 *	\date	05/06/2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */

/** preprocessing symbol to check header file */
#ifndef __DICE_DEBUG_H__
#define __DICE_DEBUG_H__

#ifndef NDEBUG

//@{
/** some variables used for debugging */
#ifndef GLOBAL_DEBUG_TRACE
#define GLOBAL_DEBUG_TRACE

extern int nGlobalDebug;
#define DTRACE(s, args...)	if (nGlobalDebug == 1) printf(s, ## args)
#define DTRACE_ON			nGlobalDebug = 1
#define DTRACE_OFF			nGlobalDebug = 0

#endif				/* GLOBAL_DEBUG_TRACE */
//@}

/** print debug information */
#define TRACE(s, args...)	do { printf(s, ## args); fflush(stdout); } while(0)

#else

//@{
/** all debug macros are empty */
#define TRACE(s, args...)
#define DTRACE(s, args...)
#define DTRACE_ON
#define DTRACE_OFF
//@}

#endif

#endif

