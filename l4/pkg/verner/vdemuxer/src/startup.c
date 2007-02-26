/*
 * \brief   Startup's and main loop for VERNER's sync component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/* STD/L4 Includes */
#include <stdio.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/dsi/dsi.h>

/* configuration */
#include "verner_config.h"

/* sync components internal functions */
#include "request_server.h"

/* cfg */
l4_ssize_t l4libc_heapsize = (VDEMUXER_HEAP_SIZE*1024*1024);
char LOG_tag[9] = "vdemuxer";

int
main (int argc, const char **argv)
{

  /* init DSI */
  dsi_init ();

  /* Main loop */
  VideoDemuxerComponent_dice_thread ();

  /* if we're here is time to panic */
  Panic ("shouldn't be here!");
  return -L4_EUNKNOWN;
}
