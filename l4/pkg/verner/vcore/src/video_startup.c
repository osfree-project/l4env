/*
 * \brief   Startup's and main loop for VERNER's core component (video pt.)
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
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/dsi/dsi.h>
#include <l4/util/macros.h>

/* configuration */
#include "verner_config.h"

/* sync components internal functions */
#include "video_request_server.h"

/* local */
#if VCORE_DO_FILTER_BENCHMARK || PREDICT_DECODING_TIME
#include "timer.h"
#endif

/* cfg */
l4_ssize_t l4libc_heapsize = (VCORE_VIDEO_HEAP_SIZE*1024*1024);
char LOG_tag[9] = "vcore_vid";

int
main (int argc, char **argv)
{

  /* init DSI */
  dsi_init ();

#if VCORE_DO_FILTER_BENCHMARK || PREDICT_DECODING_TIME
  /* init timer for benchmarking */
  init_timer ();
#endif
    
    
  /* Main loop */
  VideoCoreComponent_Video_dice_thread ();

  /* if we're here is time to panic */
  Panic ("shouldn't be here!");
  return -L4_EUNKNOWN;
}
