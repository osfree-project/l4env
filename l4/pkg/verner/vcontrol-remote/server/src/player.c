/*
 * \brief   Player User interface for  VERNER's control component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>

/* l4 */
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* local includes */
#include "arch_globals.h"
#include "arch_plugins.h"
#include <l4/vsync/functions.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include "player.h"
#include "mutex.h"

/* verner components */
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vsync/functions.h>

#include <l4/comquad/verner_gui-client.h>

/* configuration */
#include "verner_config.h"

/* mutliple instance support */
#include "minstances.h"
extern int instance_no;

/* local */
#include "helper.h"
extern connect_chain_t video_chain;
extern connect_chain_t audio_chain;

/* remote interface server functions */
#include "vcontrol-remote-server.h"

static int vid_tracks, aud_tracks;	/* noof video/audio tracks in stream */

/* types and service funtions for UI */
#include "remote-defines.h"
remote_gui_state_t gui_state;

static void
start_playing (void)
{
  /* Note these are blocking calls - so we create threads
   * to receive all possible signals 
   * after EOS the streams are closed by start_*
   */
  /* open file AFTER connection to vdemuxer is ready ! */
  VideoDemuxerComponentIntern_setVideoFileParam (video_chain.vdemuxer_thread_id,
						 gui_state.filename, gui_state.filename, 1,
						 1, PLUG_NAME_AUTODETECT,
						 PLUG_NAME_AUTODETECT);

  /* because we cannot modify the effect plugins in the GUI,
   * we cannot set them here.
   */
      
#if BUILD_RT_SUPPORT
  /* set in all components avail the requested period and timing params */
  /* demuxer */
  VideoDemuxerComponentIntern_setRTparams (video_chain.vdemuxer_thread_id,
      gui_state.rt_period,
      gui_state.rt_reservation_demux_audio,
      gui_state.rt_reservation_demux_video,
      gui_state.rt_verbose_preemption_ipc_demux);
  /* video core */
  VideoCoreComponentIntern_setVideoRTparams (video_chain.vcore_thread_id,
      gui_state.rt_period, 
      gui_state.rt_reservation_core_video_pp,
      gui_state.rt_reservation_core_video,
      gui_state.rt_verbose_preemption_ipc_core_video);
  /* audio core */
  VideoCoreComponentIntern_setAudioRTparams (audio_chain.vcore_thread_id,
      gui_state.rt_period,
      gui_state.rt_reservation_core_audio, 0,
      gui_state.rt_verbose_preemption_ipc_core_audio);
  /* sync */
  VideoSyncComponentIntern_setRTparams (video_chain.vsync_thread_id,
      gui_state.rt_period,
      gui_state.rt_reservation_sync_audio,
      gui_state.rt_reservation_sync_video,
      gui_state.rt_verbose_preemption_ipc_sync);
#endif
  
  /* build and connect video chain */
  if (vid_tracks)
    l4thread_create ((void *) build_and_connect_video_chain, &video_chain,
		     L4THREAD_CREATE_SYNC);
  /* build and connect audio chain */
  if (aud_tracks)
    l4thread_create ((void *) build_and_connect_audio_chain, &audio_chain,
		     L4THREAD_CREATE_SYNC);

  if ((audio_chain.connected) || (video_chain.connected))
  {
    gui_state.playing = 1;
    gui_state.playmode = PLAYMODE_PLAY;
  }
  else
    gui_state.playmode = PLAYMODE_STOP;
}

static void
stop_playing (void)
{
  /* 
   * if we're in pause mode - we have to set into drop mode
   * to ensure all DSI packets will be submitted and no one is blocked ...
   */
   if (gui_state.playmode & PLAYMODE_PAUSE)
	VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id,
						  PLAYMODE_DROP);
  
  /* close already opened video components */
  if (video_chain.connected)
  {
    VideoDemuxerComponentIntern_disconnect_CompressedVideoOut
      (video_chain.vdemuxer_thread_id, 0);
    VideoCoreComponentIntern_disconnect_CompressedVideoIn
      (video_chain.vcore_thread_id, 0);
    VideoCoreComponentIntern_disconnect_UncompressedVideoOut
      (video_chain.vcore_thread_id, 0);
    VideoSyncComponentIntern_disconnect_UncompressedVideoIn (video_chain.vsync_thread_id,
							     0);
  }
  /* close already opened audio components */
  if (audio_chain.connected)
  {
    VideoDemuxerComponentIntern_disconnect_CompressedAudioOut
      (audio_chain.vdemuxer_thread_id, 0);
    VideoCoreComponentIntern_disconnect_CompressedAudioIn
      (audio_chain.vcore_thread_id, 0);
    VideoCoreComponentIntern_disconnect_UncompressedAudioOut
      (audio_chain.vcore_thread_id, 0);
    VideoSyncComponentIntern_disconnect_UncompressedAudioIn (audio_chain.vsync_thread_id,
							     0);
  }

  /* ensure NOTHING is playing !! */
  /* FIXME XXX - polling is bad */
  while (1)
  {
    l4thread_sleep (500);
    if ((!audio_chain.connected) && (!video_chain.connected))
      break;
  }

  gui_state.playmode = PLAYMODE_STOP;
  gui_state.playing = 0;
}

/*
 * probe file for UI
 * set params for vdemuxer.
 * later this should be an File Open Dialog
*/
static void
file_open (const char *filename)
{
  stream_info_t stream_info;

  free(gui_state.filename);
  gui_state.filename = NULL;  // just in case someone copies code somewhere else

  /* cpy filename to global var */
  gui_state.filename = strdup(filename);

  /* indicate nothing is openend */
  gui_state.file_opened = 0;

  /* close current file and stop playing */
  if (gui_state.playing)
    stop_playing ();

  /* probe file to update gui */
  VideoDemuxerComponentIntern_probeVideoFile (video_chain.vdemuxer_thread_id, filename,
					      1, 1,
					      /*videoTrackNo, audioTrackNo, */
					      &vid_tracks, &aud_tracks,
					      &stream_info);

  /* determine PACKET_SIZE */
  if (vid_tracks > 0)
    video_chain.packet_size = vid_streaminfo2packetsize (&stream_info);
  else
  {
    LOG_Error ("couldn't get video packetsize !");
    video_chain.packet_size = 0;
  }
  if (aud_tracks > 0)
    audio_chain.packet_size = aud_streaminfo2packetsize (&stream_info);
  else
  {
    LOG_Error ("couldn't get audio packetsize !");
    audio_chain.packet_size = 0;
  }

  /* valid video or audio ? */
  if ((stream_info.vi.format == VID_FMT_NONE)
      && (stream_info.ai.format == AUD_FMT_NONE))
  {
    LOG_Error ("this seems not an correct audio or video file. open failed.\n");
    return;
  }

  /* calculate length */
  if (stream_info.vi.length >= stream_info.ai.length)
    gui_state.length = stream_info.vi.length / 1000.00;
  else
    gui_state.length = stream_info.ai.length / 1000.00;

  /* remember if src supports seeking */
  gui_state.file_seekable = stream_info.seekable;
  if (gui_state.file_seekable)
  {
    printf ("this seems seekable :)\n");
  }
  else
  {
    printf ("this seems not seekable :/ bad luck\n");
  }

  /* file open is done */
  gui_state.file_opened = 1;
}

/*
 end file open dialog 
*/

/*
 * "Event" handler functions
 */
void
verner_remote_play_component(CORBA_Object _dice_corba_obj,
    CORBA_Server_Environment *_dice_corba_env)
{
  if (! gui_state.file_opened)
    return;

  LOG("play: from playing: %s and playmode: %s (%d)",
      gui_state.playing ? "yes":"no",
      (gui_state.playmode & PLAYMODE_PAUSE) ? "pause" :
      ((gui_state.playmode & PLAYMODE_STOP) ? "stop" : 
      ((gui_state.playmode & PLAYMODE_PLAY) ? "play" : "other")),
      gui_state.playmode);
  if (gui_state.playing)
  {
    if (gui_state.playmode & PLAYMODE_PAUSE)
    {
      gui_state.playmode = PLAYMODE_PLAY;
      VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id, PLAYMODE_PLAY);
    }
    // ignore if already playing
  }
  else
  {
    // not playing anything
    start_playing();
    VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id, PLAYMODE_PLAY);
  }
}

void
verner_remote_stop_component(CORBA_Object _dice_corba_obj,
    CORBA_Server_Environment *_dice_corba_env)
{
  if (! gui_state.file_opened)
    return;

  if (gui_state.playing)
    stop_playing ();
}      

void
verner_remote_pause_component(CORBA_Object _dice_corba_obj,
    CORBA_Server_Environment *_dice_corba_env)
{
  if (! gui_state.file_opened)
    return;

  // ignore if not playing
  if (!gui_state.playing || gui_state.playmode & PLAYMODE_STOP)
    return;

  // since our remote gui does not alternate states, we
  // also don't
  if (gui_state.playmode & PLAYMODE_PAUSE)
    return;
 
  gui_state.playmode = PLAYMODE_PAUSE;
  VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id,
      PLAYMODE_PAUSE);
}

void
verner_remote_seek_component(CORBA_Object _dice_corba_obj,
    double position,
    CORBA_Server_Environment *_dice_corba_env)
{
  if (gui_state.file_seekable)
    {
      LOG("seek: %f", position);
      VideoDemuxerComponentIntern_setSeekPosition (video_chain.vdemuxer_thread_id, 1, 1,
                                                   position * 1000, 0, SEEK_ABSOLUTE);
      gui_state.position = position;
    }								        
}

void
verner_remote_setPostprocessQuality_component(CORBA_Object _dice_corba_obj,
    unsigned short quality,
    CORBA_Server_Environment *_dice_corba_env)
{
  int dummy;
  char *filterno[3] = { "1\0", "2\0", "3\0" };
  
  /* if quality is 10 then the QAP Button is pressed:
   * otherwise the scaler is used.
   */
  printf("setPostprocessQuality(%d)\n", quality);
  if (quality == 10)
    {
      /* QAP Button */
      /* switch between states by using XOR */
      gui_state.QAP ^= 1;
      /* change QAP settings in core */
      VideoCoreComponentIntern_changeQAPSettings (video_chain.vcore_thread_id,
	  gui_state.QAP, -1, -1, &dummy,
	  &dummy);
    }
  else
    {
      /* scaler */
      gui_state.QAP = 0;
      /* disable QAP in core */
      VideoCoreComponentIntern_changeQAPSettings (video_chain.vcore_thread_id,
	  gui_state.QAP, 0, quality, &dummy,
	  &dummy);
      /* if we set it manullay we use the deault filters */
      if ((quality > 0) && (quality < 4))
	{
	  VideoCoreComponentIntern_setVideoPostprocessing
	    (video_chain.vcore_thread_id, "add", "default", filterno[quality - 1]);
	  VideoCoreComponentIntern_setVideoPostprocessing
	    (video_chain.vcore_thread_id, "activate", "", "");
	}
      else
	if (quality == 0)
	  {
	    /* remove defaule filter from chain */
	    VideoCoreComponentIntern_setVideoPostprocessing
	      (video_chain.vcore_thread_id, "delete", "default", "");
	    VideoCoreComponentIntern_setVideoPostprocessing
	      (video_chain.vcore_thread_id, "deactivate", "", "");
	  }
    }
}

void
verner_remote_setVolume_component(CORBA_Object _dice_corba_obj,
    unsigned int volume,
    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("setVolume: %u\n", volume);
  VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, volume, volume);
  gui_state.mute = 0;
  gui_state.volume = volume;
}

void
verner_remote_mute_component(CORBA_Object _dice_corba_obj,
    CORBA_Server_Environment *_dice_corba_env)
{
  /* switch state using XOR */
  gui_state.mute ^= 1;
  if (gui_state.mute)
    VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, 0, 0);
  else
    VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, gui_state.volume, gui_state.volume);
}

CORBA_boolean
verner_remote_open_component(CORBA_Object _dice_corba_obj,
    const char* filename,
    double *length,
    CORBA_Server_Environment *_dice_corba_env)
{
  CORBA_boolean ret;
  
  LOG("open: %s", filename);

  file_open(filename);
  
  if(gui_state.file_opened)
    {
      ret = 1;
      *length = gui_state.length;
      LOG("open ok: length = %d", (int)*length);
    }
  else
    {
      ret = 0;
      *length = 0.0;
      LOG("open failed");
    }

  return ret;
}

#if BUILD_RT_SUPPORT
void
verner_remote_setRTparams_component(CORBA_Object _dice_corba_obj,
    unsigned int period,
    unsigned int demux_video,
    unsigned int demux_audio,
    unsigned int core_video,
    unsigned int core_video_opt,
    unsigned int core_audio,
    unsigned int sync_video,
    unsigned int sync_audio,
    CORBA_Server_Environment *_dice_corba_env)
{
  gui_state.rt_period = period;
  gui_state.rt_reservation_demux_audio = demux_audio;
  gui_state.rt_reservation_demux_video = demux_video;
  gui_state.rt_reservation_core_video_pp = core_video_opt;
  gui_state.rt_reservation_core_video = core_video;
  gui_state.rt_reservation_core_audio = core_audio;
  gui_state.rt_reservation_sync_audio = sync_audio;
  gui_state.rt_reservation_sync_video = sync_video;
}
#endif

static l4_threadid_t update_thread = L4_INVALID_ID;
static unsigned int update_rate = 250;

void
verner_remote_register_gui_component(CORBA_Object _dice_corba_obj,
    const l4_threadid_t *gui_thread,
    unsigned int gui_rate,
    CORBA_Server_Environment *_dice_corba_env)
{
  if (gui_thread)
    update_thread = *gui_thread;
  if (gui_rate > 250)
    update_rate = gui_rate;
  else
    update_rate = 250;
}

/* update thread */
static void
player_poll_thread (void)
{
  double position = 0.00;
  int currentQLevel = 0, oldQLevel = -1;
  int maxQLevel = 0, oldMaxQLevel = -1;
  CORBA_Environment env = dice_default_environment;
  if (!gui_state.playing)
    gui_state.playmode = PLAYMODE_STOP;
  while (1)
    {
      /* query position */
      VideoSyncComponentIntern_getPosition (video_chain.vsync_thread_id, &position);
      //position /= 1000.00;      /* ms to sec */
      
      /* setting current QAP levels (video only) */
      VideoCoreComponentIntern_changeQAPSettings (video_chain.vcore_thread_id, -1, -1,
	  -1, &currentQLevel,
	  &maxQLevel);
      /* don't update if nothing has changed */
      if ((currentQLevel != oldQLevel) || (maxQLevel != oldMaxQLevel))
	{
	  oldQLevel = currentQLevel;
	  oldMaxQLevel = maxQLevel;
	}

      /* if somebody registered, send him updates */
      if (!l4_is_invalid_id(update_thread))
	{
        //  LOG("Position: %d", (int)position);
          verner_remote_gui_update_update_send(&update_thread, gui_state.playmode,
                                               currentQLevel, position, &env);
          if (DICE_HAS_EXCEPTION(&env))
          {
              LOG("Error: %d", DICE_EXCEPTION_MAJOR(&env));
              if (DICE_EXCEPTION_MAJOR(&env) == CORBA_SYSTEM_EXCEPTION)
              {
                  LOG("IPC Error: 0x%x", DICE_IPC_ERROR(&env));
              }
          }

	}
      
      /* wait */
      l4thread_sleep(update_rate);
    }                             /* end while 1 */
}

/* main */
void
player_thread (void)
{
  /* init gui_state */
  bzero(&gui_state, sizeof(remote_gui_state_t));

  /* start poll thread */
  l4thread_create ((void *) player_poll_thread, NULL, L4THREAD_CREATE_ASYNC);

  /* enter mainloop */
  LOG("starting server loop");
  verner_remote_server_loop(NULL);
}
