/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/server/src/main.c
 * \brief  Socket server startup code.
 *
 * \date   15/08/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* GENERAL INCLUDES */
#include <string.h>

/* L4-SPECIFIC INCLUDES */
#include <l4/log/l4log.h>

/* *** LOCAL INCLUDES *** */
#include "server.h"

int main(int argc, char **argv) {

  int events_support = 0;
    
  if (argc > 1) {
    if (strcmp(argv[1], "--events") == 0)
      events_support = 1;
    else
      LOG_Error("Invalid command line option: %s", argv[1]);
  }

  return start_server(events_support);
}
