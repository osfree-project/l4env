/*
 * \brief   Clientlib for VERNER's sync component
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
#include <l4/util/macros.h>

/* Verner */
#include "arch_globals.h"

/* local */
#include <l4/vsync/functions.h>
#include "vsync-client.h"


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
 * Calls VideoSyncComponentIntern_start_UncompressedVideoIn(), which unlocks
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
  ret = VideoSyncComponentIntern_start_UncompressedVideoIn_call
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
 * \brief Callback function to close stream and disconnect Sync-Component. 
 * 
 * \param comp dsi_component_t
 *
 * This function calls VideoSyncComponentIntern_disconnect_UncompressedVideoIn.
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
    VideoSyncComponentIntern_disconnect_UncompressedVideoIn (thread_id, 1);

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
 * \brief Connnect Sync-Component
 * 
 * \param thread_id  ThreadID of running vsync DICE-request-server
 * \param ctrl_ds already allocated control dataspace
 * \param data_ds already allocated data dataspace
 * \param component setup dsi_component_t with callback functions and socket_ref
 *
 * Attach dataspaces to Sync-Component. The DSI-Socket is created. 
 * The work_thread also, but it's waiting for the start_signal.
 */
/*****************************************************************************/
int
VideoSyncComponentIntern_connect_UncompressedVideoIn (l4_threadid_t thread_id,
						      const l4dm_dataspace_t *
						      ctrl_ds,
						      const l4dm_dataspace_t *
						      data_ds,
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
  ret = VideoSyncComponentIntern_connect_UncompressedVideoIn_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoSyncComponentIntern_connect_UncompressedVideoIn failed (ret %d, exc %d)",
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
 * \brief Disconnect Sync-Component
 * 
 * \param thread_id  ThreadID of running vsync DICE-request-server
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
VideoSyncComponentIntern_disconnect_UncompressedVideoIn (l4_threadid_t
							 thread_id,
							 const l4_int32_t
							 close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_disconnect_UncompressedVideoIn_call
    (&thread_id, close_socket_flag, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoSyncComponentIntern_disconnect_UncompressedVideoIn failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


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
 * Calls VideoSyncComponentIntern_start_UncompressedVideoIn(), which unlocks
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
  ret = VideoSyncComponentIntern_start_UncompressedAudioIn_call
    (&thread_id, &comp->socketref, remote, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error ("__callback_audio_connect failed (ret %d, exc %d)", ret,
	       DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Callback function to close stream and disconnect Sync-Component. 
 * 
 * \param comp dsi_component_t
 *
 * This function calls VideoSyncComponentIntern_disconnect_UncompressedVideoIn.
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
    VideoSyncComponentIntern_disconnect_UncompressedAudioIn (thread_id, 1);

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
 * \brief Connnect Sync-Component
 * 
 * \param thread_id  ThreadID of running vsync DICE-request-server
 * \param ctrl_ds already allocated control dataspace
 * \param data_ds already allocated data dataspace
 * \param component setup dsi_component_t with callback functions and socket_ref
 *
 * Attach dataspaces to Sync-Component. The DSI-Socket is created. 
 * The work_thread also, but it's waiting for the start_signal.
 */
/*****************************************************************************/
int
VideoSyncComponentIntern_connect_UncompressedAudioIn (l4_threadid_t thread_id,
						      const l4dm_dataspace_t *
						      ctrl_ds,
						      const l4dm_dataspace_t *
						      data_ds,
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
  ret = VideoSyncComponentIntern_connect_UncompressedAudioIn_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoSyncComponentIntern_connect_UncompressedVideoIn failed (ret %d, exc %d)",
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
 * \brief Disconnect Sync-Component
 * 
 * \param thread_id  ThreadID of running vsync DICE-request-server
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
VideoSyncComponentIntern_disconnect_UncompressedAudioIn (l4_threadid_t
							 thread_id,
							 const l4_int32_t
							 close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_disconnect_UncompressedAudioIn_call
    (&thread_id, close_socket_flag, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoSyncComponentIntern_disconnect_UncompressedVideoIn failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}


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
VideoSyncComponentIntern_setRTparams (l4_threadid_t thread_id,
				      unsigned long period,
				      unsigned long reservation_audio,
				      unsigned long reservation_video,
				      int verbose_preemption_ipc)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_setRTparams_call
    (&thread_id, period, reservation_audio, reservation_video,
     verbose_preemption_ipc, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoSyncComponentIntern_setRTparams failed (ret %d, exc %d)",
       ret, DICE_EXCEPTION_MAJOR(&env));
    return -1;
  }
  return 0;
}

/*****************************************************************************/
/**
 * \brief Get current playback position
 * 
 * \param thread_id  ThreadID of running vsync DICE-request-server
 * \param position   Playback position in millisec
 *
 */
/*****************************************************************************/
l4_int32_t
VideoSyncComponentIntern_getPosition (l4_threadid_t thread_id,
				      double *position)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_getPosition_call
    (&thread_id, position, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
  {
    LOG_Error
      ("VideoSyncComponentIntern_getPosition_call failed (ret %d, exc %d.%d)",
       ret, DICE_EXCEPTION_MAJOR(&env), DICE_EXCEPTION_MINOR(&env));
    return -1;
  }
  return 0;
}

/*****************************************************************************/
/**
 * \brief Set playback volume
 * 
 * \param thread_id  ThreadID of running vsync DICE-request-server
 * \param left       Volume for left speaker (8bit)
 * \param right      Volume for right speaker (8bit)
 *
 */
/*****************************************************************************/
l4_int32_t
VideoSyncComponentIntern_setVolume (l4_threadid_t thread_id, int left,
				    int right)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_setVolume_call
    (&thread_id, left, right, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
    return -1;
  return 0;
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
l4_int32_t
VideoSyncComponentIntern_setPlaybackMode (l4_threadid_t thread_id, int mode)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_setPlaybackMode_call
    (&thread_id, mode, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
    return -1;
  return 0;
}

/*****************************************************************************/
/**
* \brief Set effects plugin (for instance goom)
*
* \param fx_plugin_id       id of effects plugin to use, <0 disables
*/
/*****************************************************************************/
l4_int32_t
VideoSyncComponentIntern_setFxPlugin (l4_threadid_t thread_id,
				      int fx_plugin_id)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoSyncComponentIntern_setFxPlugin_call
    (&thread_id, fx_plugin_id, &env);
  if (ret || DICE_HAS_EXCEPTION(&env))
    return -1;
  return 0;
}
