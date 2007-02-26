/*
 * \brief   Clientlib for VERNER's demuxer component
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

/* L4/Oskit includes */
#include <stdio.h>
#include <stdlib.h>		/* malloc,free */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/names/libnames.h>
#include <l4/dsi/dsi.h>
#include <l4/util/macros.h>

/* Verner */
#include "arch_globals.h"

/* local */
#include <l4/vdemuxer/functions.h>
#include "vdemuxer-client.h"


/************************/
/*  comquad interface   */
/************************/

/************************/
/*  video interface   */
/************************/


/*****************************************************************************/
/**
 * \brief Callback function to connect stream. 
 * 
 * \param comp dsi_component_t
 * \param remote remote socket reference
 *
 * Calls VideoDemuxerComponentIntern_start_CompressedVideoIn(), which unlocks
 * a mutex and let the work_thread run.
 */
/*****************************************************************************/
static int
__callback_video_connect (dsi_component_t * comp, dsi_socket_ref_t * remote)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* call server to connect */
  ret = VideoDemuxerComponentIntern_start_CompressedVideoOut_call
    (&thread_id, &comp->socketref, remote, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error ("__callback_connect failed (ret %d, exc %d)", ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Callback function to close stream and disconnect Demuxer-Component. 
 * 
 * \param comp dsi_component_t
 *
 * This function calls VideoDemuxerComponentIntern_disconnect_CompressedVideoOut.
 */
/*****************************************************************************/
static int
__callback_video_close (dsi_component_t * comp)
{
  int ret;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* close socket */
  ret =
    VideoDemuxerComponentIntern_disconnect_CompressedVideoOut (thread_id, 1);

  /* free comp->priv */
  if (comp->priv)
  {
    free (comp->priv);
    comp->priv = NULL;
  }
  return ret;
}

/*****************************************************************************/
/**
 * \brief Connnect Demuxer-Component
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param ctrl_ds already allocated control dataspace
 * \param data_ds already allocated data dataspace
 * \param component setup dsi_component_t with callback functions and socket_ref
 *
 * Attach dataspaces to Demuxer-Component. The DSI-Socket is created. 
 * The work_thread also, but it's waiting for the start_signal.
 */
/*****************************************************************************/
int
VideoDemuxerComponentIntern_connect_CompressedVideoOut (l4_threadid_t
							thread_id,
							const l4dm_dataspace_t
							* ctrl_ds,
							const l4dm_dataspace_t
							* data_ds,
							dsi_component_t *
							component)
{
  int ret;
  CORBA_Environment env = dice_default_environment;
  dsi_socket_ref_t socket_ref;

  /* share dataspaces */
  ret = l4dm_share ((l4dm_dataspace_t *) ctrl_ds, thread_id, L4DM_RW);
  if (ret < 0)
  {
    LOG_Error ("share ctrl dataspace failed: %s (%d)!", l4env_errstr (ret),
	       ret);
    return -1;
  }
  ret = l4dm_share ((l4dm_dataspace_t *) data_ds, thread_id, L4DM_RW);
  if (ret < 0)
  {
    LOG_Error ("share data dataspace failed: %s (%d)!", l4env_errstr (ret),
	       ret);
    return -1;
  }

  /* call server to connect */
  ret = VideoDemuxerComponentIntern_connect_CompressedVideoOut_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_connect_CompressedVideoOut failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }

  /* setup component descriptor */
  memcpy (&component->socketref, &socket_ref, sizeof (dsi_socket_ref_t));
  component->connect = __callback_video_connect;
  component->start = NULL;
  component->stop = NULL;
  component->close = __callback_video_close;
  /* register thread id for callback functions */
  component->priv = (void *) malloc (sizeof (l4_threadid_t));
  if (component->priv)
    memcpy (component->priv, &thread_id, sizeof (l4_threadid_t));
  return 0;
}

/*****************************************************************************/
/**
 * \brief Disconnect Demuxer-Component
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param close_socket_flag if !=0 - we close the DSI-socket 
 *
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 * Note: Flag close_socket_flag should only set != 0 in callback function for
 * dsi_stream_close.
 *
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 */
/*****************************************************************************/
int
VideoDemuxerComponentIntern_disconnect_CompressedVideoOut (l4_threadid_t
							   thread_id,
							   const l4_int32_t
							   close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoDemuxerComponentIntern_disconnect_CompressedVideoOut_call
    (&thread_id, close_socket_flag, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_disconnect_CompressedVideoOut failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


/************************/
/*  audio interface   */
/************************/


/*****************************************************************************/
/**
 * \brief Callback function to connect stream. 
 * 
 * \param comp dsi_component_t
 * \param remote remote socket reference
 *
 * Calls VideoDemuxerComponentIntern_start_CompressedVideoIn(), which unlocks
 * a mutex and let the work_thread run.
 */
/*****************************************************************************/
static int
__callback_audio_connect (dsi_component_t * comp, dsi_socket_ref_t * remote)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* call server to connect */
  ret = VideoDemuxerComponentIntern_start_CompressedAudioOut_call
    (&thread_id, &comp->socketref, remote, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error ("__callback_connect failed (ret %d, exc %d)", ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Callback function to close stream and disconnect Demuxer-Component. 
 * 
 * \param comp dsi_component_t
 *
 * This function calls VideoDemuxerComponentIntern_disconnect_CompressedVideoOut.
 */
/*****************************************************************************/
static int
__callback_audio_close (dsi_component_t * comp)
{
  int ret;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* close socket */
  ret =
    VideoDemuxerComponentIntern_disconnect_CompressedAudioOut (thread_id, 1);

  /* free comp->priv */
  if (comp->priv)
  {
    free (comp->priv);
    comp->priv = NULL;
  }
  return ret;
}

/*****************************************************************************/
/**
 * \brief Connnect Demuxer-Component
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param ctrl_ds already allocated control dataspace
 * \param data_ds already allocated data dataspace
 * \param component setup dsi_component_t with callback functions and socket_ref
 *
 * Attach dataspaces to Demuxer-Component. The DSI-Socket is created. 
 * The work_thread also, but it's waiting for the start_signal.
 */
/*****************************************************************************/
int
VideoDemuxerComponentIntern_connect_CompressedAudioOut (l4_threadid_t
							thread_id,
							const l4dm_dataspace_t
							* ctrl_ds,
							const l4dm_dataspace_t
							* data_ds,
							dsi_component_t *
							component)
{
  int ret;
  CORBA_Environment env = dice_default_environment;
  dsi_socket_ref_t socket_ref;

  /* share dataspaces */
  ret = l4dm_share ((l4dm_dataspace_t *) ctrl_ds, thread_id, L4DM_RW);
  if (ret < 0)
  {
    LOG_Error ("share ctrl dataspace failed: %s (%d)!", l4env_errstr (ret),
	       ret);
    return -1;
  }
  ret = l4dm_share ((l4dm_dataspace_t *) data_ds, thread_id, L4DM_RW);
  if (ret < 0)
  {
    LOG_Error ("share data dataspace failed: %s (%d)!", l4env_errstr (ret),
	       ret);
    return -1;
  }

  /* call server to connect */
  ret = VideoDemuxerComponentIntern_connect_CompressedAudioOut_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_connect_CompressedAudioOut failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }

  /* setup component descriptor */
  memcpy (&component->socketref, &socket_ref, sizeof (dsi_socket_ref_t));
  component->connect = __callback_audio_connect;
  component->start = NULL;
  component->stop = NULL;
  component->close = __callback_audio_close;
  /* register thread id for callback functions */
  component->priv = (void *) malloc (sizeof (l4_threadid_t));
  if (component->priv)
    memcpy (component->priv, &thread_id, sizeof (l4_threadid_t));
  return 0;
}

/*****************************************************************************/
/**
 * \brief Disconnect Demuxer-Component
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param close_socket_flag if !=0 - we close the DSI-socket 
 *
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 * Note: Flag close_socket_flag should only set != 0 in callback function for
 * dsi_stream_close.
 *
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 */
/*****************************************************************************/
int
VideoDemuxerComponentIntern_disconnect_CompressedAudioOut (l4_threadid_t
							   thread_id,
							   const l4_int32_t
							   close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoDemuxerComponentIntern_disconnect_CompressedAudioOut_call
    (&thread_id, close_socket_flag, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_disconnect_CompressedAudioOut failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
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
VideoDemuxerComponentIntern_setRTparams (l4_threadid_t thread_id,
					 unsigned long period,
					 unsigned long reservation_audio,
					 unsigned long reservation_video,
					 int verbose_preemption_ipc)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoDemuxerComponentIntern_setRTparams_call
    (&thread_id, period, reservation_audio, reservation_video,
     verbose_preemption_ipc, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_setRTparams failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}

/*****************************************************************************/
/**
 * \brief Probe file content
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
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
VideoDemuxerComponentIntern_probeVideoFile (l4_threadid_t thread_id,
					    const char *filename,
					    l4_int32_t videoTrackNo,
					    l4_int32_t audioTrackNo,
					    l4_int32_t * videoTracks,
					    l4_int32_t * audioTracks,
					    frame_ctrl_t * streaminfo)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoDemuxerComponentIntern_probeVideoFile_call
    (&thread_id, filename, videoTrackNo, audioTrackNo, videoTracks,
     audioTracks, streaminfo, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_probeVideoFile failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set parameter for demuxing a specific file
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param videoFilename File to demux video from
 * \param audioFilename File to demux video from
 * \param videoTrackNo Noof video stream within file to demux (-1 ignore!)
 * \param audioTrackNo Noof audio stream within file to demux (-1 ignore!)
 * \param videoPlugin  Name of video import plugin
 * \param audioPlugin  Name of audio import plugin
 *
 */
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_setVideoFileParam (l4_threadid_t thread_id,
					       const char *videoFilename,
					       const char *audioFilename,
					       l4_int32_t videoTrackNo,
					       l4_int32_t audioTrackNo,
					       const char *videoPlugin,
					       const char *audioPlugin)
{
  CORBA_Environment env = dice_default_environment;
  /* check sender id */
  int ret = VideoDemuxerComponentIntern_setVideoFileParam_call
    (&thread_id, videoFilename, audioFilename, videoTrackNo, audioTrackNo,
     videoPlugin, audioPlugin, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_setVideoFileParam failed (ret %d, exc %d.%d)",
       ret, DICE_EXCEPTION_MAJOR(&env), DICE_EXCEPTION_MINOR(&env));
    if ((DICE_EXCEPTION_MAJOR(&env) == CORBA_SYSTEM_EXCEPTION) &&
	(DICE_EXCEPTION_MINOR(&env) == CORBA_DICE_EXCEPTION_IPC_ERROR))
      LOG ("  ipc error: %x partner " l4util_idfmt, DICE_IPC_ERROR(&env),
	   l4util_idstr (thread_id));
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set position to seek to
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param demuxVideo  Demux video stream ? =1 demux it
 * \param demuxAudio Demux audio stream ? =1 demux it
 * \param position Position in stream in millisec to seek to
 * \param fileoffset Bytepos to seek to
 * \param whence SEEK_RELATIVE seek to (current_position + position)
 *               SEEK_ABSOLUTE seek to (start_of_file + position)
 */
/*****************************************************************************/
l4_int32_t
VideoDemuxerComponentIntern_setSeekPosition (l4_threadid_t thread_id,
					     int seekVideo,
					     int seekAudio,
					     double position, 
					     long fileoffset, int whence)
{
  CORBA_Environment env = dice_default_environment;
  /* check sender id */
  int ret = VideoDemuxerComponentIntern_setSeekPosition_call
    (&thread_id, seekVideo, seekAudio, position, fileoffset, whence, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoDemuxerComponentIntern_setSeekPosition failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}
