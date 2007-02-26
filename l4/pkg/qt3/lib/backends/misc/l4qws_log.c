/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/misc/l4qws_log.c
 * \brief  log function.
 *
 * \date   11/02/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/log_printf.h>
#include <l4/util/macros.h>
#include <l4/sys/syscalls.h>

int qt_log_drops(char *buf);
int qt_log_drops(char *buf)
{
  LOG_printf(l4util_idfmt":%s\n", l4util_idstr(l4_myself()), buf);
  return 0;
}
  
