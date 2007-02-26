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

/* L4/std libs */
#include <l4/util/getopt.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/dsi/dsi.h>
#include <stdio.h>

/* verner configuration */
#include "verner_config.h"

/* DOpE UI */
#include <dopelib.h>

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
#include "defines.h"
gui_state_t gui_state;
long app_id;

/* local */
#include "player-UI.h"

/* local configuration */
#define VCONTROL_NAME "Vcontrol"
l4_ssize_t l4libc_heapsize = (VCONTROL_HEAP_SIZE * 1024 * 1024);
char LOG_tag[9] = "vcontrol";

/* command line given args */
int QAP_on = 0;			/* default is QAP off */
int user_quality = 0;		/* quality level, default is 0 */

/* 
 * check command line args 
 */
static void
check_arguments (int argc, char **argv)
{
  char c;

  static struct option long_options[] = {
    {"nogui", no_argument, 0, 'n'},
    {"QAP", no_argument, 0, 'q'},
    {"Q0", no_argument, 0, '0'},
    {"Q1", no_argument, 0, '1'},
    {"Q2", no_argument, 0, '2'},
    {"Q3", no_argument, 0, '3'},
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
    {"silent", no_argument, 0, 's'},
    {"learn", required_argument, 0, 'l'},
    {"predict", required_argument, 0, 'p'},
#endif
    {0, 0, 0, 0}
  };

  /* read command line arguments */
  while (1)
  {
    c = getopt_long (argc, argv, "n", long_options, NULL);
    if (c <= 0)
      break;

    switch (c)
    {
    case 'n':
      LOG ("disabled GUI. Using command ling args.");
      gui_state.text_only = 1;
      break;
    case 'q':
      QAP_on = 1;
      break;
    case '0':
      user_quality = 0;
      break;
    case '1':
      user_quality = 1;
      break;
    case '2':
      user_quality = 2;
      break;
    case '3':
      user_quality = 3;
      break;
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
    case 's':
      gui_state.silent = 1;
      break;
    case 'l':
      video_chain.learn_file = optarg ? optarg : "";
      audio_chain.learn_file = optarg ? optarg : "";
      break;
    case 'p':
      video_chain.predict_file = optarg ? optarg : "";
      audio_chain.predict_file = optarg ? optarg : "";
      break;
#endif
    default:
      break;
    }
  }
}


/*
 * just a main
 */
int
main (int argc, char **argv)
{
  /* register at nameserver - if it fails - we just add the appendix "-" */
  instance_no = register_instance (VCONTROL_NAME);
  if (instance_no < 0)
  {
    LOG_Error ("Failed to register instance. Exiting.");
    return -L4_ENOTFOUND;
  }
  else
    LOGdL (DEBUG_MINSTANCES, "registered as \"%s%i\" at namesserver.",
	   VCONTROL_NAME, instance_no);

  /* first we search and find all running verner components */
  /* find vdemuxer */
  audio_chain.vdemuxer_thread_id = video_chain.vdemuxer_thread_id =
    find_instance (VDEMUXER_NAME, instance_no, 1000);
  if (l4_is_invalid_id (video_chain.vdemuxer_thread_id))
  {
    LOG_Error ("no demuxer found");
    return -L4_ENOTFOUND;
  }
  /* we need at sync */
  audio_chain.vsync_thread_id = video_chain.vsync_thread_id =
    find_instance (VSYNC_NAME, instance_no, 1000);
  if (l4_is_invalid_id (video_chain.vsync_thread_id))
  {
    LOG_Error ("no sync found");
    return -L4_ENOTFOUND;
  }
  /* we need at least one core ! */
  video_chain.vcore_thread_id =
    find_instance (VCORE_VIDEO_NAME, instance_no, 1000);
  audio_chain.vcore_thread_id =
    find_instance (VCORE_AUDIO_NAME, instance_no, 1000);
  if ((l4_is_invalid_id (audio_chain.vcore_thread_id))
      && (l4_is_invalid_id (video_chain.vcore_thread_id)))
  {
    LOG_Error ("neither audio core nor video core found");
    return -L4_ENOTFOUND;
  }

  /* init dsi */
  dsi_init ();

  /* init gui_state to default values */
  memset (&gui_state, 0, sizeof (gui_state));
  gui_state.playmode = PLAYMODE_STOP;
#if BUILD_RT_SUPPORT
  /* get default values from make config */
  gui_state.rt_period = RT_DEFAULT_PERIOD;
  gui_state.rt_reservation_demux_audio = RT_DEMUXER_AUDIO_EXEC_TIME;
  gui_state.rt_reservation_demux_video = RT_DEMUXER_VIDEO_EXEC_TIME;
  gui_state.rt_reservation_core_audio = RT_CORE_AUDIO_EXEC_TIME;
  gui_state.rt_reservation_core_video = RT_CORE_VIDEO_EXEC_TIME;
  gui_state.rt_reservation_sync_audio = RT_SYNC_AUDIO_EXEC_TIME;
  gui_state.rt_reservation_sync_video = RT_SYNC_VIDEO_EXEC_TIME;
#endif
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
  video_chain.learn_file = video_chain.predict_file = "";
  audio_chain.learn_file = audio_chain.predict_file = "";
#endif

  /* read cmdline args */
  check_arguments (argc, argv);
  if (optind) {
    /* shift the options so that later evaluation will only see the
     * ones which have not been parsed already */
    argc -= optind - 1;
    argv += optind - 1;
  }

  /* wait to ensure dope is ready */
  l4thread_sleep (1000);

  /* init dope */
  if (!gui_state.text_only)
  {
    dope_init ();
    /* register DOpE-application */
    app_id = dope_init_app ("VControl");
  }
  else
    app_id = -1;

  /* start player UI */
  player_UI_thread (argc, argv);

  return 0;
}
