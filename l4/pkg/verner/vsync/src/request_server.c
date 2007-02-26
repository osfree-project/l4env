/*
 * \brief   IPC server for VERNER's sync component
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


/* L4/DROPS includes */
#include <stdio.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* verner */
#include "arch_globals.h"
#include "arch_plugins.h"

/* local */
#include "vsync-server.h"
#include "work_loop.h"
#include "arch_globals.h"
#include "request_server.h"
#include "functions.h"
#include "metronome.h"
#include "playerstatus.h"

/* configuration */
#include "verner_config.h"

/* multiple instances support */
#include "minstances.h"
int instance_no = -1;

/* 
 * these are handles :)
 * to be flexible for later extension they can be passed to the IDL 
 * and to the clientlib 
 */
static control_struct_t video_control;
static control_struct_t audio_control;

/*****************************************************************************/
/**
* \brief Thread which handles DICE event loop
* 
*/
/*****************************************************************************/
int
VideoSyncComponent_dice_thread (void)
{

  /* register at nameserver - if it fails - we just add the appendix "-" */
  instance_no = register_instance (VSYNC_NAME);
  if (instance_no < 0)
  {
    LOG_Error ("Failed to register instance. Exiting.");
    return instance_no;
  }
  else
    LOGdL (DEBUG_MINSTANCES, "registered as \"%s%i\" at namesserver.",
	   VSYNC_NAME, instance_no);

  /* zero control structs */
  memset (&video_control, 0, sizeof (control_struct_t));
  memset (&audio_control, 0, sizeof (control_struct_t));

  /* IDL server loop */
  VideoSyncComponentIntern_server_loop (NULL);

  Panic ("left server loop - this is really bad.");
  return 0;
}

/************************/
/*  comquad interface   */
/************************/


/************************/
/*  video interface   */
/************************/


/*****************************************************************************/
/**
* \brief Connnect Sync-Component
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param component setup dsi_component_t with callback functions and socket_ref
*
* Attach datascpaces to Sync-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
long
VideoSyncComponentIntern_connect_UncompressedVideoIn_component (CORBA_Object
								_dice_corba_obj,
								const
								l4dm_dataspace_t
								* ctrl_ds,
								const
								l4dm_dataspace_t
								* data_ds,
								dsi_socket_ref_t
								* socketref,
								CORBA_Server_Environment
								*
								_dice_corba_env)
{
  /* check if we have already a video sender connected */
  if (video_control.in_use)
  {
    LOG_Error ("video connection already opened!");
    return -1;
  }
  video_control.in_use = 1;

  /* set control struct to defaults ! */
  video_control.stream_type = STREAM_TYPE_VIDEO;
  video_control.shutdown_work_thread = 0;	/* flag if we shutdown work thread */
  video_control.start_sem = L4SEMAPHORE_LOCKED;	/* semaphore to start work_thread */
  video_control.running_sem = L4SEMAPHORE_UNLOCKED;	/* semaphore if the work_thread is running */
#if BUILD_RT_SUPPORT
  video_control.running_preempt = L4SEMAPHORE_UNLOCKED;	/* semaphore if the preempter thread is running */
#endif
  video_control.create_sem = L4SEMAPHORE_UNLOCKED;	/* semaphore if we're just creating the thread */
  video_control.plugin_ctrl.mode = PLUG_MODE_EXPORT;	/* of course we want to export */
  return receiver_open_socket (&video_control, ctrl_ds, data_ds, socketref);
}

/*****************************************************************************/
/**
* \brief Start Sync-Component's work thread
*
* \param local local socket reference
* \param remote remote socket reference
*
* unlocks a mutex and let the work_thread run.
*/
/*****************************************************************************/
long
VideoSyncComponentIntern_start_UncompressedVideoIn_component (CORBA_Object
							      _dice_corba_obj,
							      const
							      dsi_socket_ref_t
							      * local,
							      const
							      dsi_socket_ref_t
							      * remote,
							      CORBA_Server_Environment
							      *
							      _dice_corba_env)
{
  /* check if we have already a video sender connected */
  if (!video_control.in_use)
  {
    LOG_Error ("video connection not opened!");
    return -1;
  }
  return receiver_start (&video_control, (dsi_socket_ref_t *) local,
			 (dsi_socket_ref_t *) remote);
}


/*****************************************************************************/
/**
 * \brief Disconnect Sync-Component
 * 
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 */
/*****************************************************************************/
long
  VideoSyncComponentIntern_disconnect_UncompressedVideoIn_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* check if we have already a video sender connected */
  if (!video_control.in_use)
  {
    LOG_Error ("video connection not opened!");
    return -1;
  }
  /* free use indicator */
  if (close_socket_flag)
    video_control.in_use = 0;
  return receiver_close (&video_control, close_socket_flag);
}


/************************/
/*  audio interface   */
/************************/


/*****************************************************************************/
/**
* \brief Connnect Sync-Component
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param component setup dsi_component_t with callback functions and socket_ref
*
* Attach datascpaces to Sync-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
long
VideoSyncComponentIntern_connect_UncompressedAudioIn_component (CORBA_Object
								_dice_corba_obj,
								const
								l4dm_dataspace_t
								* ctrl_ds,
								const
								l4dm_dataspace_t
								* data_ds,
								dsi_socket_ref_t
								* socketref,
								CORBA_Server_Environment
								*
								_dice_corba_env)
{
  /* check if we have already a video sender connected */
  if (audio_control.in_use)
  {
    LOG_Error ("audio connection already opened!");
    return -1;
  }
  audio_control.in_use = 1;

  /* set control struct to defaults ! */
  audio_control.stream_type = STREAM_TYPE_AUDIO;
  audio_control.shutdown_work_thread = 0;	/* flag if we shutdown work thread */
  audio_control.start_sem = L4SEMAPHORE_LOCKED;	/* semaphore to start work_thread */
  audio_control.running_sem = L4SEMAPHORE_UNLOCKED;	/* semaphore if the work_thread is running */
#if BUILD_RT_SUPPORT
  audio_control.running_preempt = L4SEMAPHORE_UNLOCKED;	/* semaphore if the preempter thread is running */
#endif
  audio_control.create_sem = L4SEMAPHORE_UNLOCKED;	/* semaphore if we're just creating the thread */
  audio_control.plugin_ctrl.mode = PLUG_MODE_EXPORT;	/* of course we want to export */
  /* fx plugin */
  audio_control.fx_plugin_ctrl.mode = PLUG_MODE_EXPORT;	/* fx plugins use also export */

  return receiver_open_socket (&audio_control, ctrl_ds, data_ds, socketref);
}

/*****************************************************************************/
/**
* \brief Start Sync-Component's work thread
*
* \param local local socket reference
* \param remote remote socket reference
*
* unlocks a mutex and let the work_thread run.
*/
/*****************************************************************************/
long
VideoSyncComponentIntern_start_UncompressedAudioIn_component (CORBA_Object
							      _dice_corba_obj,
							      const
							      dsi_socket_ref_t
							      * local,
							      const
							      dsi_socket_ref_t
							      * remote,
							      CORBA_Server_Environment
							      *
							      _dice_corba_env)
{
  /* check if we have already a audio sender connected */
  if (!audio_control.in_use)
  {
    LOG_Error ("audio connection not opened!");
    return -1;
  }
  return receiver_start (&audio_control, (dsi_socket_ref_t *) local,
			 (dsi_socket_ref_t *) remote);
}


/*****************************************************************************/
/**
 * \brief Disconnect Sync-Component
 * 
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 */
/*****************************************************************************/
long
  VideoSyncComponentIntern_disconnect_UncompressedAudioIn_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* check if we have already a audio sender connected */
  if (!audio_control.in_use)
  {
    LOG_Error ("audio connection not opened!");
    return -1;
  }
  /* free use indicator */
  if (close_socket_flag)
    audio_control.in_use = 0;
  return receiver_close (&audio_control, close_socket_flag);
}






/************************/
/* functional interface */
/************************/


/*****************************************************************************/
/**
 * \brief Set period for rt mode
 *
 * \param period period in microseconds
 * \param reservation_audio  reservation time for ONE audio chunk [microsecond]
 * \param reservation_video  reservation time for ONE video frame [microsecond]
 * \param verbose_preemption_ipc  show each recv. preeption ipc ?
 *
 */
/*****************************************************************************/
long
VideoSyncComponentIntern_setRTparams_component (CORBA_Object
						_dice_corba_obj,
						unsigned long period,
						unsigned long reservation_audio,
						unsigned long reservation_video,
						int verbose_preemption_ipc,
						CORBA_Server_Environment *
						_dice_corba_env)
{
#if BUILD_RT_SUPPORT
  audio_control.rt_verbose_pipc = video_control.rt_verbose_pipc =
    (int) verbose_preemption_ipc;
  audio_control.rt_period = video_control.rt_period = period;
  audio_control.rt_reservation = reservation_audio;
  video_control.rt_reservation = reservation_video;
#endif
  return 0;
}

/*****************************************************************************/
/**
 * \brief Get current playback position
 * 
 * \param position   Playback position in millisec
 *
 */
/*****************************************************************************/
long
  VideoSyncComponentIntern_getPosition_component
  (CORBA_Object _dice_corba_obj, double *position,
   CORBA_Server_Environment * _dice_corba_env)
{
  *position = metronome_get_position ();
  return 0;
}

/*****************************************************************************/
/**
 * \brief Set playback volume
 * 
 * \param left       Volume for left speaker (8bit)
 * \param right      Volume for right speaker (8bit)
 *
 */
/*****************************************************************************/
long
  VideoSyncComponentIntern_setVolume_component
  (CORBA_Object _dice_corba_obj, int left, int right,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* remember volume for next video start */
  audio_control.volume_left = left;
  audio_control.volume_right = right;

  /* set volume if already running */
  if (audio_control.out_setVolume)
  {
    /* set info for OSD */
    snprintf (video_control.plugin_ctrl.info, 32, "Vol: %i",
	      (left + right) / 2);
    /* and yes, audio can print text via fx plugins */
    snprintf (audio_control.plugin_ctrl.info, 32, "Vol: %i",
	      (left + right) / 2);
    /* notify audio work loop */
    return audio_control.out_setVolume (NULL /*dummy */ , left, right);
  }

  /* failed */
  return -L4_ENOTSUPP;
}


/*****************************************************************************/
/**
* \brief Set playback mode (Pause, Play, FF)
* 
* \param mode       Playback command (see arch_globals.h)
*
* To stop playback use  disconnect_Uncompressed[Video/Audio]In.
*/
/*****************************************************************************/
long
  VideoSyncComponentIntern_setPlaybackMode_component
  (CORBA_Object _dice_corba_obj, int mode,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* redirect to event handler */
  switch (mode)
  {
  case PLAYMODE_PLAY:
    VideoSyncComponent_event (SYNC_EVENT_PLAY, NULL);
    break;
  case PLAYMODE_PAUSE:
    VideoSyncComponent_event (SYNC_EVENT_PAUSE, NULL);
    break;
  case PLAYMODE_STOP:
    VideoSyncComponent_event (SYNC_EVENT_STOP, NULL);
    break;
  case PLAYMODE_DROP:
    VideoSyncComponent_event (SYNC_EVENT_DROP, NULL);
    break;
  }
  /* done */
  return 0;
}




/*****************************************************************************/
/**
* \brief event handler called by export plugins to allow key/mouse events
* 
* \param event       Event
* \param osd_text    Text to display via OSD
*
*/
/*****************************************************************************/
int
VideoSyncComponent_event (int event, char *osd_text)
{
  /* use an dummy object */
  CORBA_Object dice_corba_obj = NULL;

  /* set info for OSD */
  if (osd_text)
  {
    snprintf (video_control.plugin_ctrl.info, 32, osd_text);
    snprintf (audio_control.fx_plugin_ctrl.info, 32, osd_text);
  }

  switch (event)
  {
  case SYNC_EVENT_STOP:
    /* set player mode, just to be sure */
    set_player_mode (PLAYMODE_STOP);
    /* disconnect */
    VideoSyncComponentIntern_disconnect_UncompressedVideoIn_component
      (dice_corba_obj, 0, NULL);
    VideoSyncComponentIntern_disconnect_UncompressedAudioIn_component
      (dice_corba_obj, 0, NULL);
    break;
  case SYNC_EVENT_PLAY:
    /* set player mode */
    set_player_mode (PLAYMODE_PLAY);
    /* set info for OSD */
    snprintf (video_control.plugin_ctrl.info, 32, "Play");
    snprintf (audio_control.fx_plugin_ctrl.info, 32, "Play");
    break;
  case SYNC_EVENT_PAUSE:
    /* set player mode */
    set_player_mode (PLAYMODE_PAUSE);
    /* set info for OSD */
    snprintf (video_control.plugin_ctrl.info, 32, "Pause");
    snprintf (audio_control.fx_plugin_ctrl.info, 32, "Pause");
    break;
  case SYNC_EVENT_DROP:
    /* set player mode */
    set_player_mode (PLAYMODE_DROP);
    break;
  }
  return 0;
}



/*****************************************************************************/
/**
* \brief Set effects plugin (for instance goom)
*
* \param fx_plugin_id       id of effects plugin to use, <0 disables
*/
/*****************************************************************************/
long
  VideoSyncComponentIntern_setFxPlugin_component
  (CORBA_Object _dice_corba_obj, int fx_plugin_id,
   CORBA_Server_Environment * _dice_corba_env)
{
  audio_control.fx_plugin_id = fx_plugin_id;
  return 0;
}
