/*!
 * \file   log/lib/src/logliblinux.c
 * \brief  Output function for liblog printf without server
 *
 * \date   02/13/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>
#include "internal.h"

FILE* LOG_stdlog = 0;

// output a string, but do not add a trailing newline
static void outs(const char *s)
{
  if (LOG_stdlog == 0)
    LOG_stdlog = stderr;
  fputs(s, LOG_stdlog);
}

//void (*LOG_outstring)(const char*) __attribute__ ((weak));
void (*LOG_outstring)(const char *) = outs;

void LOG_flush(void)
{
  if (LOG_stdlog == 0)
    LOG_stdlog = stderr;
  fflush(LOG_stdlog);
}
