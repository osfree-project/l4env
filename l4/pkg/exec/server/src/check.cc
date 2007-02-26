/* $Id$ */
/**
 * \file        exec/server/src/check.cc
 * \brief       Some assert like functions
 *
 * \date        10/2000
 * \author      Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "check.h"

#include <l4/env/errno.h>

#include <stdio.h>
#include <stdarg.h>

/** Check error and give message if != 0 */
int
check(int error, const char *format, ...)
{
  if (error)
    {
      va_list list;
      
      printf("Error %d (%s) ", error, l4env_errstr(error));
      va_start(list, format);
#ifdef USE_OSKIT
      vprintf(format, (oskit_va_list)list);
#else
      vprintf(format, list);
#endif
      va_end(list);
      printf("\n");
    }
  
  return error;
}

