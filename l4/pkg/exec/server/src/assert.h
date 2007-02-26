/*!
 * \file	assert.h
 * \brief	Some assert macros
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __ASSERT_H_
#define __ASSERT_H_

#include <l4/sys/kdebug.h>
#include <stdio.h>

/** My own panic macro */
#define Panic(format, args...) do {		\
    printf(format "\n" , ## args);		\
    enter_kdebug("Panic");			\
} while (0)

/** My own error macro */
#define Error(format, args...) do {		\
    printf("In %s:\n"format "\n", __PRETTY_FUNCTION__ , ## args); \
    enter_kdebug("Error");			\
} while (0)

#ifdef NDEBUG

#define Assert(expr) ((void)0)

#else

#define Assert(expr) do {			\
    if (!(expr)) { 				\
	printf(#expr "\n");			\
	Panic("[%s:%d]: Assertion failed",	\
	    __FILE__, __LINE__);		\
    }						\
} while (0)

#endif /* NDEBUG */

#endif /* __L4_EXEC_SERVER_ASSERT_H */

