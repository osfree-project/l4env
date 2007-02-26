/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4util/include/macros.h
 * \brief  Some useful generic macros
 *
 * \date   03/09/2002
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
#ifndef _L4UTIL_MACROS_H
#define _L4UTIL_MACROS_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>

/*****************************************************************************
 *** generic macros
 *****************************************************************************/

/* print message and enter kernel debugger */
#ifndef Panic

// Don't include <stdlib.h> here, leads to trouble.
// Don't use exit() here since we want to terminate ASAP.
// We might be executed in context of the region manager.
EXTERN_C_BEGIN
void _exit(int status) __attribute__ ((__noreturn__));
EXTERN_C_END

#  define Panic(args...) do                                      \
                           {                                     \
                             LOGL(args);                         \
                             LOG_flush();                        \
                             enter_kdebug("PANIC, 'g' for exit");\
                             _exit(-1);                          \
                           }                                     \
                         while (1)
#endif

/* assertion */
#ifndef Assert
#  define Assert(expr) do                                        \
                         {                                       \
                           if (!(expr))                          \
                             {                                   \
                               LOG_printf(#expr "\n");           \
                               Panic("Assertion failed");        \
                             }                                   \
                         }                                       \
                       while (0)
#endif

/* enter kernel debugger */
#ifndef Kdebug
#  define Kdebug(args...)  do                                    \
                             {                                   \
                               LOGL(args);                       \
                               LOG_flush();                      \
                               enter_kdebug("KD");               \
                             }                                   \
                           while (0)
#endif

/*****************************************************************************
 *** debug stuff (to be removed, use LOG* macros instead!)
 *****************************************************************************/

/* we use our own debug macros */
#ifdef KDEBUG
#  undef KDEBUG
#endif
#ifdef ASSERT
#  undef ASSERT
#endif
#ifdef PANIC
#  undef PANIC
#endif

#ifdef DEBUG

#define KDEBUG(args...) do                                   \
                          {                                  \
                            LOGL(args);                      \
                            LOG_flush();                     \
                            enter_kdebug("KD");              \
                          }                                  \
                        while (0)

#ifdef DEBUG_ASSERTIONS
#  define ASSERT(expr)    Assert(expr)
#else
#  define ASSERT(expr)    do {} while (0)
#endif

#ifdef DEBUG_ERRORS
#  define PANIC(format, args...) Panic(format, ## args)
#else
#  define PANIC(args...)  do {} while (0)
#endif

#else /* !DEBUG */

#define KDEBUG(args...) do {} while (0)
#define ASSERT(expr)    do {} while (0)
#define PANIC(args...)  do {} while (0)

#endif /* !DEBUG */

#endif /* !_L4UTIL_MACROS_H */
