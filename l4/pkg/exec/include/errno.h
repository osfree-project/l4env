/* $Id$ */
/**
 * \file	exec/include/ARCH-x86/errno.h
 * \brief	L4 execution layer public error codes
 *
 * \date	08/18/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _L4_EXEC_ERRNO_H
#define _L4_EXEC_ERRNO_H

#define L4_EXEC_BADFORMAT	2001	/* invalid file format */
#define L4_EXEC_BADARCH		2002	/* invalid architecture */
#define L4_EXEC_CORRUPT		2003	/* file is corrupt */
#define L4_EXEC_NOSTANDARD	2004	/* defined entry point not found */
#define L4_EXEC_LINK		2005	/* errors while linking */
#define L4_EXEC_INTERPRETER	2006	/* contains INTERP section */

asm(".globl l4exec_err_strings_sym; .type l4exec_err_strings_sym, %object");

#endif /* _L4_EXEC_ERRNO_H */
