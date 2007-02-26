/*
 * \brief   Startup for VERNER's control component
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

#include <stdio.h>

#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>

/* configuration */
#include "verner_config.h"

#include <l4/dsi/dsi.h>

#include "player.h"

/* verner components */
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vsync/functions.h>

/* multiple instances support */
#include "minstances.h"
int instance_no = -1;

/* types and functions for building video and audio chain */
#include "helper.h"
connect_chain_t video_chain;
connect_chain_t audio_chain;

/* types and service funtions for UI */
#include "remote-defines.h"

/* cfg */
l4_ssize_t l4libc_heapsize = (VCONTROL_HEAP_SIZE*1024*1024);
char LOG_tag[9] = "vcontrol";

/* globals */
int use_player = 1;

#define VCONTROL_NAME "Vcontrol"

int
main (int argc, char **argv)
{
  /* register at nameserver - if it fails - we just add the appendix "-" */
  instance_no = register_instance(VCONTROL_NAME);
  if(instance_no < 0)
  {
  	LOG_Error("Failed to register instance. Exiting.");
	return -L4_ENOTFOUND;
  }
  else
    LOGdL (DEBUG_MINSTANCES,"registered as \"%s%i\" at namesserver.", VCONTROL_NAME, instance_no);

  /* first we search and find all running verner components */
  
  /* find vdemuxer */
  audio_chain.vdemuxer_thread_id = video_chain.vdemuxer_thread_id = find_instance(VDEMUXER_NAME, instance_no, 10000);
  if (l4_is_invalid_id(video_chain.vdemuxer_thread_id))
    {
      LOG_Error("no demuxer found");
      return -L4_ENOTFOUND;
    }
  /* we need at sync */
  audio_chain.vsync_thread_id = video_chain.vsync_thread_id = find_instance(VSYNC_NAME, instance_no, 10000);
  if (l4_is_invalid_id(video_chain.vsync_thread_id))
    {
      LOG_Error("no sync found");
      return -L4_ENOTFOUND;
    }
  /* we need at least one core ! */
  video_chain.vcore_thread_id = find_instance(VCORE_VIDEO_NAME, instance_no, 10000);
  audio_chain.vcore_thread_id = find_instance(VCORE_AUDIO_NAME, instance_no, 10000);
  if ((l4_is_invalid_id(audio_chain.vcore_thread_id)) && (l4_is_invalid_id(video_chain.vcore_thread_id)))
    {
      LOG_Error("neither audio core nor video core found");
      return -L4_ENOTFOUND;
    }
  
  /* init dsi */
  dsi_init ();

#if BUILD_RT_SUPPORT
  /* get default values from make config */
  gui_state.rt_period = RT_DEFAULT_PERIOD;
  gui_state.rt_reservation_demux_audio = RT_DEMUXER_AUDIO_EXEC_TIME;
  gui_state.rt_reservation_demux_video = RT_DEMUXER_VIDEO_EXEC_TIME;
  gui_state.rt_reservation_core_audio = RT_CORE_AUDIO_EXEC_TIME;
  gui_state.rt_reservation_core_video = RT_CORE_VIDEO_EXEC_TIME;
  gui_state.rt_reservation_core_video_pp = RT_CORE_VIDEO_POSTPROC_EXEC_TIME;
  gui_state.rt_reservation_sync_audio = RT_SYNC_AUDIO_EXEC_TIME;
  gui_state.rt_reservation_sync_video = RT_SYNC_VIDEO_EXEC_TIME;
#endif

  /* start player UI */
  player_thread ();

  return 0;
}
