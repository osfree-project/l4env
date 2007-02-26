/* $Id$ */
/**
 * \file	con/server/include/con_macros.h
 * \brief	some macros
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _CON_MACROS_H
#define _CON_MACROS_H

/* L4 includes */
#include <l4/sys/kdebug.h>

/* local includes */
#include "con_log.h"

/* print error message and panic */
#undef PANIC
#define PANIC(format, args...)                          \
  do {                                                  \
    printf("%s():\n", __FUNCTION__);                    \
    Panic(format, ## args);                             \
    exit(-1);						\
  } while(0)

#endif /* !_CON_MACROS_H */
