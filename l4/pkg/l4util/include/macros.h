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

/* standard / OSKit includes */
#include <stdio.h>

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>

/*****************************************************************************
 *** generic macros
 *****************************************************************************/

/* print message */
#ifndef Msg
#  define Msg(format, args...)   printf(format, ## args)
#endif

/* print error message */
#ifndef Error
#  define Error(format, args...) do                                      \
                                   {                                     \
                                     printf("[%s:%d] in function %s:\n", \
                                            __FILE__, __LINE__,          \
                                            __FUNCTION__);               \
                                     printf("Error: " format, ## args);  \
                                     printf("\n");                       \
                                     LOG_flush();                        \
                                   }                                     \
                                 while (0)
#endif

/* print message and enter kernel debugger */
#ifndef Panic
#  define Panic(format, args...) do                                      \
                                   {                                     \
                                     printf("[%s:%d] in function %s:\n", \
                                            __FILE__, __LINE__,          \
                                            __FUNCTION__);               \
                                     printf(format, ## args);            \
                                     printf("\n");                       \
                                     LOG_flush();                        \
                                     enter_kdebug("PANIC");              \
                                   }                                     \
                                 while (0)
#endif

/* assertion */
#ifndef Assert
#  define Assert(expr) do                                                \
                         {                                               \
                           if (!(expr))                                  \
                             {                                           \
                               printf(#expr);                            \
                               printf("\n");                             \
                               Panic("Assertion failed");                \
                             }                                           \
                         }                                               \
                       while (0)
#endif

/* enter kernel debugger */
#ifndef Kdebug
#  define Kdebug(args...)  do                                            \
                             {                                           \
                               LOGL(args);                               \
                               LOG_flush();                              \
                               enter_kdebug("KD");                       \
                             }                                           \
                           while (0)
#endif

/*****************************************************************************
 *** debug stuff (to be removed, use LOG* macros instead!)
 *****************************************************************************/

/* we use our own debug macros */
#ifdef DMSG
#  undef DMSG
#endif
#ifdef INFO
#  undef INFO
#endif
#ifdef KDEBUG
#  undef KDEBUG
#endif
#ifdef ASSERT
#  undef ASSERT
#endif
#ifdef ERROR
#  undef ERROR
#endif
#ifdef PANIC
#  undef PANIC
#endif

#ifdef DEBUG

#define DMSG(args...)   printf( args)

//#define INFO(args...)   LOGL( args)
#define INFO(args...)   do                                   \
                          {                                  \
			    printf("%s:%d (%s,"IdFmt"): ",   \
                                   __FILE__,__LINE__,        \
		                   __FUNCTION__,             \
		                   IdStr(l4_myself()));      \
			           printf( args);            \
	                  }                                  \
                        while (0)

#define KDEBUG(args...) do                                   \
                          {                                  \
                            INFO( args);                     \
                            printf("\n");                    \
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
#  define ERROR(format, args...) Error(format, ## args)
#  define PANIC(format, args...) Panic(format, ## args)
#else
#  define ERROR(args...)  do {} while (0)
#  define PANIC(args...)  do {} while (0)
#endif

#else /* !DEBUG */

#define DMSG(args...)   do {} while (0)
#define INFO(args...)   do {} while (0)
#define KDEBUG(args...) do {} while (0)
#define ASSERT(expr)    do {} while (0)
#define ERROR(args...)  do {} while (0)
#define PANIC(args...)  do {} while (0)

#endif /* !DEBUG */

#endif /* !_L4UTIL_MACROS_H */
