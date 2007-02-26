/*
 * \brief   Player User interface for  VERNER's control component
 * \date    2004-05-14
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


/* env */
#include <stdlib.h>
#include <l4/util/rand.h>

/* dope */
#include <dopelib.h>

/* verner */
#include "arch_globals.h"
#include "arch_plugins.h"
#include "verner_config.h"

/* verner components */
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vsync/functions.h>

/* mutliple instance support */
#include "minstances.h"
extern int instance_no;

/* local */
#include "helper.h"
extern connect_chain_t video_chain;
extern connect_chain_t audio_chain;
#include "playlist.h"
#include "defines.h"
#include "extctrl.h"
#include "player-UI.h"


/* -----------------------------------
 * helper functions
 * ----------------------------------- */
static int start_playing (int command, char *filename);
static void stop_playing (void);

/* -----------------------------------
 * main window
 * ----------------------------------- */
static void gui_updater_thread (void);
static void gui_callback (dope_event * e, void *arg);


 /* -----------------------------------
  * helper functions
  * ----------------------------------- */
int
start_playing (int command, char *filename)
{
  int vid_tracks, aud_tracks, rand_no;
  pl_entry_t *entry_to_play = NULL;
  double stream_length;

  switch (command)
  {
  case CMD_FORCE_STREAM:
    /* force playback of given file */
    if (filename == NULL)
    {
      Panic ("GUI error - forced but no filename given");
      return -L4_EUNKNOWN;
    }
    entry_to_play = playlist_get_entry_by_filename (filename);
    break;
  case CMD_NEXT_STREAM:
    /* play next file in list */
    break;
  case CMD_PREV_STREAM:
    /* play prev file in list */
    break;
  case CMD_RAND_STREAM:
    /* play random file from list */
    if (pl_entries)
      rand_no = l4util_rand () * pl_entries / L4_RAND_MAX;
    else
      rand_no = 1;
    entry_to_play = playlist_get_entry_by_id (rand_no);
    break;
  case CMD_FIRST_STREAM:
    /* play first file from list */
    entry_to_play = playlist_get_entry_by_id (1);
    break;
  default:
    Panic ("GUI error - not a valid command");
    return -L4_EUNKNOWN;
  }

  /* check for valid entry */
  if ((entry_to_play == NULL) || (entry_to_play->filename == NULL))
  {
    setInfo ("no streams in playlist");
    return -L4_ENOTFOUND;
  }


  /* close current file and stop playing */
  if (gui_state.just_playing)
    stop_playing ();


  /*
   * probe file for UI
   * set params for vdemuxer.
   */
  if (VideoDemuxerComponentIntern_probeVideoFile
      (video_chain.vdemuxer_thread_id, entry_to_play->filename, 1, 1,
       /*videoTrackNo, audioTrackNo, */
       &vid_tracks, &aud_tracks, &gui_state.stream_info))
  {
    setInfo ("no demuxer found");
    playlist_gui_highlight (-1);
    setPlaymode (PLAYMODE_STOP);
    return -L4_ENOTFOUND;
  }


  /* determine PACKET_SIZE */
  if (vid_tracks > 0)
    video_chain.packet_size =
      vid_streaminfo2packetsize (&gui_state.stream_info);
  else
    video_chain.packet_size = 0;

  if (aud_tracks > 0)
    audio_chain.packet_size =
      aud_streaminfo2packetsize (&gui_state.stream_info);
  else
    audio_chain.packet_size = 0;

  /* valid video or audio ? */
  if ((gui_state.stream_info.vi.format == VID_FMT_NONE)
      && (gui_state.stream_info.ai.format == AUD_FMT_NONE))
  {
    setInfo ("invalid stream/file");
    LOG_Error ("this seems not an correct audio or video file. open failed.");
    playlist_gui_highlight (-1);
    setPlaymode (PLAYMODE_STOP);
    return -L4_EUNKNOWN;
  }

  /* adjust seek scaler */
  stream_length =
    (gui_state.stream_info.vi.length >=
     gui_state.stream_info.ai.length) ? (gui_state.stream_info.vi.
					 length /
					 1000.00) : (gui_state.
						     stream_info.ai.
						     length / 1000.00);
  mutex_lock (gui_state.seek_lock);
  vdope_cmd (app_id, "main_scale_seek.set(-value 0.00)");
  /* default is to seek via position in msec */
  gui_state.seek_via_filesize = 0;
  if (stream_length > 0)
  {
    vdope_cmdf (app_id, "main_scale_seek.set(-from 0.00 -to %i)", (int) stream_length);	//fixme: no floating point accepted !!
  }
  else
  {
    if (gui_state.stream_info.filesize != 0)
    {
      /* set slider via filesize */
      vdope_cmdf (app_id, "main_scale_seek.set(-from 0.00 -to %lu)",
		  gui_state.stream_info.filesize);
      gui_state.seek_via_filesize = 1;
      LOG ("Activated seek via filesize cause stream length is unknown!");
    }
    else
      /* length is totally unknown */
      vdope_cmd (app_id, "main_scale_seek.set(-from 0.00 -to 20000000)");
  }
  mutex_unlock (gui_state.seek_lock);

  /* update playlist window */
  playlist_gui_update (entry_to_play->id,
		       strlen (gui_state.stream_info.info) <=
		       0 ? NULL : gui_state.stream_info.info, stream_length);

  /* set file params for vdemuxer */
  VideoDemuxerComponentIntern_setVideoFileParam (video_chain.
						 vdemuxer_thread_id,
						 entry_to_play->filename,
						 entry_to_play->filename, 1,
						 1, PLUG_NAME_AUTODETECT,
						 PLUG_NAME_AUTODETECT);


  /* 
   * if we have an effect pluging in extctrl enabled, 
   * but we have an video stream - we disable fx for performance reasons
   * (user may override in extctrl)
   */
  if (vid_tracks)
  {
    /* disable fx */
    VideoSyncComponentIntern_setFxPlugin
      (video_chain.vsync_thread_id, FX_PLUG_ID_NONE);
  }
  else
  {
    /* enable fx plugin */
    VideoSyncComponentIntern_setFxPlugin
      (video_chain.vsync_thread_id, gui_state.fx_plugin);
  }


#if BUILD_RT_SUPPORT
  /* set in all components avail the requested period and timing params */
  /* demuxer */
  VideoDemuxerComponentIntern_setRTparams (video_chain.vdemuxer_thread_id,
					   gui_state.rt_period,
					   gui_state.
					   rt_reservation_demux_audio,
					   gui_state.
					   rt_reservation_demux_video,
					   gui_state.
					   rt_verbose_preemption_ipc_demux);

  /* video core */
  VideoCoreComponentIntern_setVideoRTparams (video_chain.vcore_thread_id,
					     gui_state.rt_period, 0,
					     gui_state.
					     rt_reservation_core_video,
					     gui_state.
					     rt_verbose_preemption_ipc_core_video);
  /* audio core */
  VideoCoreComponentIntern_setAudioRTparams (audio_chain.vcore_thread_id,
					     gui_state.rt_period,
					     gui_state.
					     rt_reservation_core_audio, 0,
					     gui_state.
					     rt_verbose_preemption_ipc_core_audio);
  /* sync */
  VideoSyncComponentIntern_setRTparams (video_chain.vsync_thread_id,
					gui_state.rt_period,
					gui_state.rt_reservation_sync_audio,
					gui_state.rt_reservation_sync_video,
					gui_state.
					rt_verbose_preemption_ipc_sync);
#endif

  /*
   * we create threads to avoid blocking GUI commands and updates 
   */

  /* build and connect video chain */
  if (vid_tracks)
    if (!l4thread_create
	((void *) build_and_connect_video_chain, &video_chain,
	 L4THREAD_CREATE_SYNC))
      Panic ("video-chain thread build failed\n");

  /* build and connect audio chain */
  if (aud_tracks)
    if (!l4thread_create
	((void *) build_and_connect_audio_chain, &audio_chain,
	 L4THREAD_CREATE_SYNC))
      Panic ("audio-chain thread build failed\n");

  /* we're here, so at least we're trying to open now :) */
  if ((audio_chain.connected) || (video_chain.connected))
  {
    /* maybe we connected sucessfull or we are just trying */
    /* so set gui state  */
    setInfo (strlen (gui_state.stream_info.info) <=
	     0 ? entry_to_play->filename : gui_state.stream_info.info);

    gui_state.current_pl_entry = entry_to_play;
    gui_state.just_playing = 1;
    setPlaymode (PLAYMODE_PLAY);
    playlist_gui_highlight (entry_to_play->id);
    /* ok */
    return 0;
  }
  else
  {
    /* sure it failed to connect */
    setInfo ("unable to play");
    playlist_gui_highlight (-1);
    setPlaymode (PLAYMODE_STOP);
    LOGl ("unable to play");
    return -L4_EUNKNOWN;
  }
}


void
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
    VideoSyncComponentIntern_disconnect_UncompressedVideoIn (video_chain.
							     vsync_thread_id,
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
    VideoSyncComponentIntern_disconnect_UncompressedAudioIn (audio_chain.
							     vsync_thread_id,
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

  setPlaymode (PLAYMODE_STOP);
  gui_state.just_playing = 0;
  gui_state.current_pl_entry = NULL;
  playlist_gui_highlight (-1);
}


/* -----------------------------------
 * thread for updating UI
 * ----------------------------------- */
void
gui_updater_thread (void)
{
  double position, stream_length;
  int currentQLevel = 0, oldQLevel = -1;
  int maxQLevel = 0, oldMaxQLevel = -1;

  while (1)
  {

    /* if currently playing */
    if (gui_state.just_playing)
    {
      /* check if we reached to end of stream */
      if ((!audio_chain.connected) && (!video_chain.connected))
      {
	/* send an event to event manager, he should now look if we should play further */
	event_manager (EV_EOF, NULL, 0);
	continue;
      }

      /* get postion in current stream from vsync */
      VideoSyncComponentIntern_getPosition (video_chain.vsync_thread_id,
					    &position);
      position /= 1000.00;	/* ms to sec */
      /* update time */
      stream_length =
	(int) (gui_state.stream_info.vi.length >=
	       gui_state.stream_info.ai.length) ? (gui_state.stream_info.vi.
						   length /
						   1000.00) : (gui_state.
							       stream_info.ai.
							       length /
							       1000.00);

      vdope_cmdf (app_id,
		  "main_label_time.set(-text \"[%.2d:%.2d/%.2d:%.2d]\")",
		  (int) position / 60, (int) position % 60,
		  (int) stream_length / 60, (int) stream_length % 60);

    }
    else
    {
      vdope_cmd (app_id, "main_label_time.set(-text \"\")");
      position = 0;
    }

    /*  set scale position */
    if (!mutex_is_locked (gui_state.seek_lock))
    {
      mutex_lock (gui_state.seek_lock);
      vdope_cmdf (app_id, "main_scale_seek.set(-value %i)", (int) position);
      mutex_unlock (gui_state.seek_lock);
    }

    /* setting current QAP levels (video only) */
    VideoCoreComponentIntern_changeQAPSettings (video_chain.vcore_thread_id,
						-1, -1, -1, &currentQLevel,
						&maxQLevel);
    /* don't update if nothing has changed */
    if ((currentQLevel != oldQLevel) || (maxQLevel != oldMaxQLevel))
    {
      vdope_cmdf (app_id, "extctrl_scale_quality.set(-value %i)",
		  currentQLevel);
      vdope_cmdf (app_id, "extctrl_scale_quality.set(-from 0.00 -to  %d.10)",
		  maxQLevel);
      oldQLevel = currentQLevel;
      oldMaxQLevel = maxQLevel;
    }

    /* wait */
    l4thread_sleep (1000);
  }				/* end while 1 */
}



/* -----------------------------------
 * main window:
 * the unviversal callback handler
 * ----------------------------------- */
void
gui_callback (dope_event * e, void *arg)
{
  char result[16];		/* vdope_req result */
  int cmd = (int) arg;
  switch (cmd)
  {
  case CMD_CLEAR:
    /* clear playlist */
    event_manager (EV_PLAYLIST_CLEAR, NULL, 0);
    break;
  case CMD_PLAY:
    /* Play button */
    event_manager (EV_PLAY, NULL, 0);
    break;
  case CMD_STOP:
    /* Stop button */
    event_manager (EV_STOP, NULL, 0);
    break;
  case CMD_PAUSE:
    /* Pause button */
    event_manager (EV_PAUSE, NULL, 0);
    break;
  case CMD_BW:
    /* BW button */
    event_manager (EV_BWD, NULL, 0);
    break;
  case CMD_FW:
    /* FW button */
    event_manager (EV_FWD, NULL, 0);
    break;
  case CMD_SEEK:
    /* seek scaler */
    /* lock when begin moving */
    if ((e->type == EVENT_TYPE_PRESS)
	&& (!mutex_is_locked (gui_state.seek_lock)))
      mutex_lock (gui_state.seek_lock);
    /* end seeking */
    if (e->type != EVENT_TYPE_PRESS)	//better would be an SLID_Event...
    {
      if (gui_state.stream_info.seekable)
      {
	/* request offset by DOpE */
	char result[16];
	double position;	/*  pos in sec * 1000 = msec */
	vdope_req (app_id, result, 16, "main_scale_seek.value");
	position = strtod (result, (char **) NULL);
	if (gui_state.seek_via_filesize == 0)
	{
	  /* seek with msec-position */
	  LOGdL (DEBUG_SYNC, "want to seek to position %s.\n", result);
	  VideoDemuxerComponentIntern_setSeekPosition (video_chain.
						       vdemuxer_thread_id, 1,
						       1,
						       (double) position *
						       1000, 0,
						       SEEK_ABSOLUTE);
	}
	else
	{
	  /* seek via filesize */
	  LOGdL (DEBUG_SYNC, "want to seek to byte position %s.\n", result);
	  VideoDemuxerComponentIntern_setSeekPosition (video_chain.
						       vdemuxer_thread_id, 1,
						       1, 0.00,
						       (int) position,
						       SEEK_ABSOLUTE);
	}
      }
      /* unlock at end of moving the slider */
      mutex_unlock (gui_state.seek_lock);
    }
    break;
  case CMD_VOL:
    /* volume scaler */
    {
      int vol;
      vdope_req (app_id, result, 16, "main_scale_vol.value");
      vol = (int) strtod (result, (char **) NULL);
      vdope_cmdf (app_id, "main_scale_vol.set(-value %i)", vol);
      VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, vol,
					  vol);
      gui_state.mute = 0;
      vdope_cmdf (app_id, "main_btn_mute.set(-state %i)", gui_state.mute);
      break;
    }
  case CMD_MUTE:
    /* mute button */
    gui_state.mute ^= 1;
    if (gui_state.mute)
      VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, 0, 0);
    else
    {
      int vol;
      vdope_req (app_id, result, 16, "main_scale_vol.value");
      vol = (int) strtod (result, (char **) NULL);
      vdope_cmdf (app_id, "main_scale_vol.set(-value %i)", vol);
      VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, vol,
					  vol);
    }
    vdope_cmdf (app_id, "main_btn_mute.set(-state %i)", gui_state.mute);
    break;
  case CMD_RANDOM:
    /* random button */
    gui_state.random ^= 1;
    vdope_cmdf (app_id, "main_btn_rand.set(-state %i)", gui_state.random);
    break;
  case CMD_REPEAT:
    /* repeat button */
    gui_state.repeat ^= 1;
    vdope_cmdf (app_id, "main_btn_repeat.set(-state %i)", gui_state.repeat);
    break;

  }				/* end case */

}



/* 
 * -----------------------------------
 * implementation
 * ----------------------------------- 
 */

int
event_manager (int command, char *string, int integer)
{
  /* next entry */
  pl_entry_t *next_entry = NULL;

  /* lock because we must serialize these events */
  mutex_lock (gui_state.event_lock);

  dumpEvent;

  switch (command)
  {
  case EV_PLAYLIST_ADD:
    /* add stream/file to playlist */
    playlist_add (string, NULL, 0.00);
    break;
  case EV_PLAYLIST_REM:
    /* remove stream/file to playlist */
    /* should we remove the file currently playing? then stop before */
    if ((gui_state.just_playing) && (gui_state.current_pl_entry)
	&& (gui_state.current_pl_entry->id == integer))
    {
      setPlaymode (PLAYMODE_STOP);
      stop_playing ();
      setInfo ("removed & stopped");
    }
    playlist_remove (integer);
    break;
  case EV_PLAYLIST_CLEAR:
    /* clear playlist */
    setPlaymode (PLAYMODE_STOP);
    if (gui_state.just_playing)
      stop_playing ();
    playlist_clear ();
    break;
  case EV_STOP:
    /* stop playing */
    /* if pause -> play */
    if (gui_state.just_playing)
      stop_playing ();
    setPlaymode (PLAYMODE_STOP);
    setInfo ("player ready");
    break;
  case EV_PLAY:
    /* begin playing if currently in paused */
    if ((gui_state.just_playing) && (gui_state.playmode & PLAYMODE_PAUSE))
    {
      setPlaymode (PLAYMODE_PLAY);
      VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id,
						gui_state.playmode);
    }
    /* is there someting in playlist - then we can play */
    else if (pl_entries)
    {
      setPlaymode (PLAYMODE_PLAY);
      VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id,
						gui_state.playmode);
      /* if a filename is given we force to play it */
      if (string)
      {
	if (gui_state.just_playing)
	  stop_playing ();
	start_playing (CMD_FORCE_STREAM, string);
      }
      /* if we don't already play, we do it now */
      else if (!gui_state.just_playing)
      {
	start_playing (gui_state.random ? CMD_RAND_STREAM : CMD_FIRST_STREAM,
		       NULL);
      }
    }
    else
      setInfo ("no streams in playlist");
    break;
  case EV_PAUSE:
    /* pause playing */

    /* ignore if !PLAYING */
    if ((gui_state.playmode & PLAYMODE_STOP) || (!gui_state.just_playing))
      break;

    /* toggle pause */
    if (gui_state.playmode & PLAYMODE_PAUSE)
      setPlaymode (PLAYMODE_PLAY);
    else if (gui_state.playmode & PLAYMODE_PLAY)
      setPlaymode (PLAYMODE_PAUSE);
    else
      Panic ("GUI BUG - report it");

    /* send command to vsync */
    VideoSyncComponentIntern_setPlaybackMode (video_chain.vsync_thread_id,
					      gui_state.playmode);

    break;
  case EV_BWD:
  case EV_FWD:
  case EV_EOF:
    /* 
     * backward or forward in playlist or 
     * end of current stream reached 
     * check if we're at end of playlist or we should repeat
     */

    /* ignore if stopped, but ensure it's stopped */
    if ((gui_state.playmode & PLAYMODE_STOP) && (!video_chain.connected)
	&& (!audio_chain.connected))
    {
      if (gui_state.just_playing)
	stop_playing ();
      playlist_gui_highlight (-1);
      break;
    }

    if (command == EV_BWD)
      /* backward - select previous file */
      next_entry =
	gui_state.current_pl_entry ? gui_state.current_pl_entry->prev : NULL;
    else
      /* forward or EOF - select next file */
      next_entry =
	gui_state.current_pl_entry ? gui_state.current_pl_entry->next : NULL;

    if ((!next_entry) && (!gui_state.repeat) && (!gui_state.random))
      /* no file follows in playlist and repeat/random off */
    {
      setInfo (command == EV_BWD ? "begin of playlist" : "end of playlist");
      if (gui_state.just_playing)
	stop_playing ();
      gui_state.just_playing = 0;
      setPlaymode (PLAYMODE_STOP);
      playlist_gui_highlight (-1);
    }
    else
      /* either a file follows in playlist or repeat from start */
    {
      if ((next_entry) && (next_entry->filename))
	/* a valid file follows */
      {
	start_playing (gui_state.random ? CMD_RAND_STREAM : CMD_FORCE_STREAM,
		       next_entry->filename);
      }
      else
	/* invalid file follows - so start from beginning or random */
      {
	start_playing (gui_state.random ? CMD_RAND_STREAM : CMD_FIRST_STREAM,
		       NULL);
      }
    }

    break;
  }

  /* lock */
  mutex_unlock (gui_state.event_lock);
  return 0;
}


/* -----------------------------------
 * just like the good old main 
 * ----------------------------------- */
void
player_UI_thread (int argc, char **argv)
{
  int i;

  /* set seek and event unlocked */
  gui_state.event_lock = create_mutex (MUTEX_UNLOCKED);
  gui_state.seek_lock = create_mutex (MUTEX_UNLOCKED);

  /* build GUI */
  if (!gui_state.text_only)
  {
#include "player-UI.dpi"
  }
  /* bind control buttons */
  vdope_bind (app_id, "main_btn_bw", "release", gui_callback,
	      (void *) CMD_BW);
  vdope_bind (app_id, "main_btn_play", "release", gui_callback,
	      (void *) CMD_PLAY);
  vdope_bind (app_id, "main_btn_pause", "release", gui_callback,
	      (void *) CMD_PAUSE);
  vdope_bind (app_id, "main_btn_stop", "release", gui_callback,
	      (void *) CMD_STOP);
  vdope_bind (app_id, "main_btn_fw", "release", gui_callback,
	      (void *) CMD_FW);
  /* seek-scaler binding */
  vdope_bind (app_id, "main_scale_seek", "press", gui_callback, (void *) CMD_SEEK);	//begin to move
  vdope_bind (app_id, "main_scale_seek", "slid", gui_callback, (void *) CMD_SEEK);	//release
  /* random and repeat bindings */
  vdope_bind (app_id, "main_btn_rand", "press", gui_callback,
	      (void *) CMD_RANDOM);
  vdope_bind (app_id, "main_btn_repeat", "press", gui_callback,
	      (void *) CMD_REPEAT);

  /* default volume */
  vdope_bind (app_id, "main_scale_vol", "slid", gui_callback,
	      (void *) CMD_VOL);
  vdope_cmdf (app_id, "main_scale_vol.set(-value %i)", 0xf0);
  vdope_bind (app_id, "main_btn_mute", "press", gui_callback,
	      (void *) CMD_MUTE);
  vdope_cmd (app_id, "main_btn_mute.set(-state 0)");
  VideoSyncComponentIntern_setVolume (video_chain.vsync_thread_id, 0xf0,
				      0xf0);

  /* edit title to show instance no. if necessary */
  if (!instance_no)
  {
    vdope_cmd (app_id, "main_win.set(-title \"VERNER\")");
  }
  else
  {
    vdope_cmdf (app_id, "main_win.set(-title \"VERNER (%i)\")", instance_no);
    vdope_cmdf (app_id, "main_win.set(-x %i)", (instance_no * 20) + 70);
  }

  /* playlist */
  vdope_bind (app_id, "main_btn_pl", "press", playlist_show_callback,
	      (void *) NULL);
  vdope_bind (app_id, "playlist_btn_add", "commit", playlist_change_callback,
	      (void *) CMD_ADD);
  vdope_bind (app_id, "playlist_btn_rmall", "commit", gui_callback,
	      (void *) CMD_CLEAR);
  vdope_bind (app_id, "playlist_entry", "commit", playlist_change_callback,
	      (void *) CMD_ADD);

  /* add files from cmd line to playlist (accept options) */
  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] != '-')
      event_manager (EV_PLAYLIST_ADD, argv[i], 0);
  }

  /* extra controls */
  ec_init ();

  /* defauls */
  vdope_cmd (app_id,
	     "extctrl_scale_quality.set(-value 0 -from 0.00 -to 0.00)");
  vdope_cmd (app_id, "extctrl_btn_qap.set(-state 0)");

  /* now show main window */
  vdope_cmd (app_id, "main_win.open()");

  /* start gui updater  */
  l4thread_create ((void *) gui_updater_thread, NULL, L4THREAD_CREATE_ASYNC);

  /* enter mainloop in dope mode */
  if (!gui_state.text_only)
    dope_eventloop (app_id);
  /* else play until end of day or error */
  else if (pl_entries)
  {
    gui_state.repeat = 1;
    setPlaymode (PLAYMODE_PLAY);
    /* if we don't already play, we do it now */
    start_playing (CMD_FIRST_STREAM, NULL);
    /* wait for ever */
    l4thread_sleep_forever ();
  }
  else
    /* no entries to play */
    LOG ("can't play cause got nothing to play :( Bye.");

  /* clear playlist */
  event_manager (EV_PLAYLIST_CLEAR, NULL, 0);

  /* kill mutexes */
  destroy_mutex (gui_state.event_lock);
  destroy_mutex (gui_state.seek_lock);
}
