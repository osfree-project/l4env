/*
 * \brief   IPC server for VERNER's demuxer component
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
#include <stdlib.h>		/* realloc */

/* verner */
#include "arch_globals.h"
#include "arch_plugins.h"

/* local */
#include "vdemuxer-server.h"
#include "work_loop.h"
#include "arch_globals.h"
#include "request_server.h"
#include "functions.h"
#include "container.h"
#include "types.h"

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

/************************/
/*  helper functions    */
/************************/

/*****************************************************************************/
/**
* \brief Thread which handles DICE event loop
* 
*/
/*****************************************************************************/
int
VideoDemuxerComponent_dice_thread (void)
{

  /* register at nameserver - if it fails - we just add the appendix "-" */
  instance_no = register_instance (VDEMUXER_NAME);
  if (instance_no < 0)
  {
    LOG_Error ("Failed to register instance. Exiting.");
    return instance_no;
  }
  else
    LOGdL (DEBUG_MINSTANCES, "registered as \"%s%i\" at namesserver.",
	   VDEMUXER_NAME, instance_no);

  /* clear structs */
  memset (&video_control, 0, sizeof (control_struct_t));
  memset (&audio_control, 0, sizeof (control_struct_t));

  /* IDL server loop */
  VideoDemuxerComponentIntern_server_loop (NULL);

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
* \brief Connnect Demuxer-Component
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param component setup dsi_component_t with callback functions and socket_ref
*
* Attach datascpaces to Demuxer-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
l4_int32_t
  VideoDemuxerComponentIntern_connect_CompressedVideoOut_component
  (CORBA_Object _dice_corba_obj, const l4dm_dataspace_t * ctrl_ds,
   const l4dm_dataspace_t * data_ds, dsi_socket_ref_t * socketref,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* check if we have already a video sender connected */
  if (video_control.in_use)
  {
    LOGL ("VDemuxer: video connection already opened!");
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
  video_control.seek = 0;	/* we don't want to seek at startup */
  return sender_open_socket (&video_control, ctrl_ds, data_ds, socketref);
}

/*****************************************************************************/
/**
* \brief Start Demuxer-Component's work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* unlocks a mutex and let the work_thread run.
*/
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_start_CompressedVideoOut_component (CORBA_Object
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
    LOGL ("VDemuxer: video connection not opened!");
    return -1;
  }
  return sender_start (&video_control, (dsi_socket_ref_t *) local,
		       (dsi_socket_ref_t *) remote);
}


/*****************************************************************************/
/**
 * \brief Disconnect Demuxer-Component
 * 
 * \param close_socket_flag if !=0 - we close the DSI-socket 
 *
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 * Note: Flag close_socket_flag should only set != 0 in callback function for
 * dsi_stream_close.
 */
/*****************************************************************************/
l4_int32_t
  VideoDemuxerComponentIntern_disconnect_CompressedVideoOut_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* check if we have already a video sender connected */
  if (!video_control.in_use)
  {
    LOGL ("VDemuxer: video connection not opened!");
    return -1;
  }
  /* free use indicator */
  if (close_socket_flag)
  {
    video_control.in_use = 0;
    /* free filename */
    if (video_control.plugin_ctrl.filename)
      free (video_control.plugin_ctrl.filename);
    video_control.plugin_ctrl.filename = NULL;
  }
  return sender_close (&video_control, close_socket_flag);
}


/************************/
/*  audio interface   */
/************************/


/*****************************************************************************/
/**
* \brief Connnect Demuxer-Component
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param component setup dsi_component_t with callback functions and socket_ref
*
* Attach datascpaces to Demuxer-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
l4_int32_t
  VideoDemuxerComponentIntern_connect_CompressedAudioOut_component
  (CORBA_Object _dice_corba_obj, const l4dm_dataspace_t * ctrl_ds,
   const l4dm_dataspace_t * data_ds, dsi_socket_ref_t * socketref,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* check if we have already a audio sender connected */
  if (audio_control.in_use)
  {
    LOGL ("VDemuxer: audio connection already opened!");
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
  audio_control.seek = 0;	/* we don't want to seek at startup */
  return sender_open_socket (&audio_control, ctrl_ds, data_ds, socketref);
}

/*****************************************************************************/
/**
* \brief Start Demuxer-Component's work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* unlocks a mutex and let the work_thread run.
*/
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_start_CompressedAudioOut_component (CORBA_Object
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
    LOGL ("VDemuxer: audio connection not opened!");
    return -1;
  }
  return sender_start (&audio_control, (dsi_socket_ref_t *) local,
		       (dsi_socket_ref_t *) remote);
}


/*****************************************************************************/
/**
 * \brief Disconnect Demuxer-Component
 * 
	 * \param close_socket_flag if !=0 - we close the DSI-socket 
	 *
	 * This function ensures work_thread is closed and nobody is working 
	 * with DSI-sockets. Afterwards the DSI-socket is closed.
	 * Note: Flag close_socket_flag should only set != 0 in callback function for
	 * dsi_stream_close.
 */
/*****************************************************************************/
l4_int32_t
  VideoDemuxerComponentIntern_disconnect_CompressedAudioOut_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* check if we have already a audio sender connected */
  if (!audio_control.in_use)
  {
    LOGL ("VDemuxer: audio connection not opened!");
    return -1;
  }
  /* free use indicator */
  if (close_socket_flag)
  {
    audio_control.in_use = 0;
    /* free filename */
    if (audio_control.plugin_ctrl.filename)
      free (audio_control.plugin_ctrl.filename);
    audio_control.plugin_ctrl.filename = NULL;
  }
  return sender_close (&audio_control, close_socket_flag);
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
l4_int32_t
VideoDemuxerComponentIntern_setRTparams_component (CORBA_Object
						   _dice_corba_obj,
						   l4_uint32_t period,
						   l4_uint32_t
						   reservation_audio,
						   l4_uint32_t
						   reservation_video,
						   l4_int32_t
						   verbose_preemption_ipc,
						   CORBA_Server_Environment *
						   _dice_corba_env)
{
#if BUILD_RT_SUPPORT
  audio_control.rt_verbose_pipc = video_control.rt_verbose_pipc =
    (int) verbose_preemption_ipc;
  audio_control.rt_period = video_control.rt_period = period;
  audio_control.rt_reservation = reservation_audio;
  video_control.rt_reservation = reservation_video;
  audio_control.rt_preempt_count = video_control.rt_preempt_count = 0;
#endif
  return 0;
}

/*****************************************************************************/
/**
 * \brief Probe file content
 * 
 * \param filename File to probe
 * \param videoTrackNo Noof video stream within file to probe
 * \param audioTrackNo Noof audio stream within file to probe
 * \retval videoTracks Noof video streams the file contains
 * \retval audioTracks Noof audio streams the file contains
 * \retval streaminfo  information about probed streams
 *
 */
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_probeVideoFile_component (CORBA_Object
						      _dice_corba_obj,
						      const char *filename,
						      l4_int32_t videoTrackNo,
						      l4_int32_t audioTrackNo,
						      l4_int32_t *
						      videoTracks,
						      l4_int32_t *
						      audioTracks,
						      frame_ctrl_t *
						      streaminfo,
						      CORBA_Server_Environment
						      * _dice_corba_env)
{
  containerProbeVideoFile (filename, videoTrackNo, audioTrackNo, videoTracks,
			   audioTracks, streaminfo);
  return 0;
}

/*****************************************************************************/
/**
 * \brief Set parameter for demuxing a specific file
 * 
 * \param filename File to demux
 * \param videoTrackNo Noof video stream within file to demux (-1 ignore!)
 * \param audioTrackNo Noof audio stream within file to demux (-1 ignore!)
 *
 */
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_setVideoFileParam_component (CORBA_Object
							 _dice_corba_obj,
							 const char
							 *videoFilename,
							 const char
							 *audioFilename,
							 l4_int32_t
							 videoTrackNo,
							 l4_int32_t
							 audioTrackNo,
							 const char
							 *videoPlugin,
							 const char
							 *audioPlugin,
							 CORBA_Server_Environment
							 * _dice_corba_env)
{
  /* put submitted information into internal control structs */
  int filename_length;

  /*
   * set filename:
   *  we're allocating buffers for filename and copy filename into
   *  realloc would do it, but it doesn't check for size>0 :/
   */
  /* indicate we havn't a valid one */
  if (video_control.plugin_ctrl.filename)
    free (audio_control.plugin_ctrl.filename);
  video_control.plugin_ctrl.filename = NULL;

  /* first video part */
  filename_length = strlen (videoFilename);
  if (filename_length > 0)
  {
    video_control.plugin_ctrl.filename = malloc (filename_length + 1);
    /* copy filename */
    memcpy (video_control.plugin_ctrl.filename, videoFilename,
	    filename_length + 1);
  }

  /* indicate we havn't a valid one */
  if (audio_control.plugin_ctrl.filename)
    free (audio_control.plugin_ctrl.filename);
  audio_control.plugin_ctrl.filename = NULL;

  /* now audio part */
  filename_length = strlen (audioFilename);
  if (filename_length > 0)
  {
    audio_control.plugin_ctrl.filename = malloc (filename_length + 1);
    /* copy filename */
    memcpy (audio_control.plugin_ctrl.filename, audioFilename,
	    filename_length + 1);
  }

  /* set trackNo */
  video_control.plugin_ctrl.trackNo = videoTrackNo;
  audio_control.plugin_ctrl.trackNo = audioTrackNo;

  /* set plugins */
  if (!strcasecmp (videoPlugin, PLUG_NAME_AUTODETECT))
    video_control.plugin_id = -1;
  else
    video_control.plugin_id =
      find_plugin_by_name (videoPlugin, PLUG_MODE_IMPORT, STREAM_TYPE_VIDEO);
  if (!strcasecmp (audioPlugin, PLUG_NAME_AUTODETECT))
    audio_control.plugin_id = -1;
  else
    audio_control.plugin_id =
      find_plugin_by_name (audioPlugin, PLUG_MODE_IMPORT, STREAM_TYPE_AUDIO);


  return 0;
}


/*****************************************************************************/
/**
 * \brief Set position to seek to
 * 
 * \param demuxVideo  Demux video stream ? =1 demux it
 * \param demuxAudio Demux audio stream ? =1 demux it
 * \param position Position in stream in millisec to seek to
 * \param fileoffset Bytepos to seek to
 * \param whence SEEK_RELATIVE seek to (current_position + position)
 *               SEEK_ABSOLUTE seek to (start_of_file + position)
 */
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_setSeekPosition_component (CORBA_Object
						       _dice_corba_obj,
						       l4_int32_t seekVideo,
						       l4_int32_t seekAudio,
						       double position,
						       l4_int32_t fileoffset,
						       l4_int32_t whence,
						       CORBA_Server_Environment
						       * _dice_corba_env)
{
  /* we want to seek ?! */
  audio_control.seek = seekAudio;
  video_control.seek = seekVideo;
  /* seek to */
  audio_control.seek_position = video_control.seek_position = position;
  audio_control.seek_fileoffset = video_control.seek_fileoffset = fileoffset;
  audio_control.seek_whence = video_control.seek_whence = whence;
  return 0;
}
