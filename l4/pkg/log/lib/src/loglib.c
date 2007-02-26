/*!
 * \file   log/lib/src/loglib.c
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
#include <l4/sys/kdebug.h>
#include "internal.h"

// output a string, but do not add a trailing newline
static void outs(const char *s)
{
  /* use outchar instead of outstring for latency reasons */
  while (*s)
    outchar(*s++);
}

//void (*LOG_outstring)(const char*) __attribute__ ((weak));
void (*LOG_outstring)(const char *) = outs;

/* With the loglib, we just have the stdlog stream */
void LOG_flush(void)
{
  LOG_printf_flush();
}
