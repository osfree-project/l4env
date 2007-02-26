/*
 * \brief   Clientlib for VERNER's audio core component
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
#include <l4/vcore/functions_audio.h>
#include "vcore_audio-client.h"




/*****************************************************************************/
/**
 * \brief Callback function to connect outgoing stream. 
 * 
 * \param comp dsi_component_t
 * \param remote remote socket reference
 *
 * Calls VideoCoreComponentIntern_start_UncompressedAudioIn(), which unlocks
 * a mutex and let the work_thread run.
 */
/*****************************************************************************/
static int
__callback_connect_outgoing (dsi_component_t * comp,
			     dsi_socket_ref_t * remote)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;


  /* call server to connect */
  ret = VideoCoreComponentIntern_start_UncompressedAudioOut_call
    (&thread_id, &comp->socketref, remote, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error ("__callback_connect_outgoing failed (ret %d, exc %d)", ret,
	       env.major);
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Callback function to close outgoing stream and disconnect Vcore-Component. 
 * 
 * \param comp dsi_component_t
 *
 * This function calls VideoCoreComponentIntern_disconnect_UncompressedAudioOut.
 */
/*****************************************************************************/
static int
__callback_close_outgoing (dsi_component_t * comp)
{
  int ret;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* close socket */
  ret =
    VideoCoreComponentIntern_disconnect_UncompressedAudioOut (thread_id, 1);

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
 * \brief Connnect Vcore-Component
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param ctrl_ds already allocated control dataspace
 * \param data_ds already allocated data dataspace
 * \param component setup dsi_component_t with callback functions and socket_ref
 *
 * Attach dataspaces to Vcore-Component. The DSI-Socket is created. 
 * The work_thread also, but it's waiting for the start_signal.
 */
/*****************************************************************************/
int
VideoCoreComponentIntern_connect_UncompressedAudioOut (l4_threadid_t
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
  ret = VideoCoreComponentIntern_connect_UncompressedAudioOut_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_connect_UncompressedAudioOut failed (ret %d, exc %d)",
       ret, env.major);
    return -1;
  }

  /* setup component descriptor */
  memcpy (&component->socketref, &socket_ref, sizeof (dsi_socket_ref_t));
  component->connect = __callback_connect_outgoing;
  component->start = NULL;
  component->stop = NULL;
  component->close = __callback_close_outgoing;
  /* register thread id for callback functions */
  component->priv = (void *) malloc (sizeof (l4_threadid_t));
  if (component->priv)
    memcpy (component->priv, &thread_id, sizeof (l4_threadid_t));

  return 0;
}

/*****************************************************************************/
/**
 * \brief Disconnect Vcore-Component
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
VideoCoreComponentIntern_disconnect_UncompressedAudioOut (l4_threadid_t
							  thread_id,
							  const l4_int32_t
							  close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_disconnect_UncompressedAudioOut_call
    (&thread_id, close_socket_flag, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_disconnect_UncompressedAudioOut failed (ret %d, exc %d)",
       ret, env.major);
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Callback function to connect incoming stream. 
 * 
 * \param comp dsi_component_t
 * \param remote remote socket reference
 *
 * Calls VideoCoreComponentIntern_start_CompressedAudioIn(), which unlocks
 * a mutex and let the work_thread run.
 */
/*****************************************************************************/
static int
__callback_connect_incoming (dsi_component_t * comp,
			     dsi_socket_ref_t * remote)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* call server to connect */
  ret = VideoCoreComponentIntern_start_CompressedAudioIn_call
    (&thread_id, &comp->socketref, remote, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error ("__callback_connect failed (ret %d, exc %d)", ret, env.major);
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Callback function to close incoming stream and disconnect Vcore-Component. 
 * 
 * \param comp dsi_component_t
 *
 * This function calls VideoCoreComponentIntern_disconnect_CompressedAudioIn.
 */
/*****************************************************************************/
static int
__callback_close_incoming (dsi_component_t * comp)
{
  int ret;

  /* get threadID out of component_t */
  l4_threadid_t thread_id;
  thread_id = *(l4_threadid_t *) comp->priv;

  /* close socket */
  ret = VideoCoreComponentIntern_disconnect_CompressedAudioIn (thread_id, 1);

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
 * \brief Connnect Vcore-Component
 * 
 * \param thread_id  ThreadID of running vdemuxer DICE-request-server
 * \param ctrl_ds already allocated control dataspace
 * \param data_ds already allocated data dataspace
 * \param component setup dsi_component_t with callback functions and socket_ref
 *
 * Attach dataspaces to Vcore-Component. The DSI-Socket is created. 
 * The work_thread also, but it's waiting for the start_signal.
 */
/*****************************************************************************/
int
VideoCoreComponentIntern_connect_CompressedAudioIn (l4_threadid_t thread_id,
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
  ret = VideoCoreComponentIntern_connect_CompressedAudioIn_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_connect_CompressedAudioIn failed (ret %d, exc %d)",
       ret, env.major);
    return -1;
  }

  /* setup component descriptor */
  memcpy (&component->socketref, &socket_ref, sizeof (dsi_socket_ref_t));
  component->connect = __callback_connect_incoming;
  component->start = NULL;
  component->stop = NULL;
  component->close = __callback_close_incoming;
  /* register thread id for callback functions */
  component->priv = (void *) malloc (sizeof (l4_threadid_t));
  if (component->priv)
    memcpy (component->priv, &thread_id, sizeof (l4_threadid_t));

  return 0;
}

/*****************************************************************************/
/**
 * \brief Disconnect Vcore-Component
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
VideoCoreComponentIntern_disconnect_CompressedAudioIn (l4_threadid_t
						       thread_id,
						       const l4_int32_t
						       close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_disconnect_CompressedAudioIn_call
    (&thread_id, close_socket_flag, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_disconnect_CompressedAudioIn failed (ret %d, exc %d)",
       ret, env.major);
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
 * \param reservation_video  (unused in audio core)
 * \param verbose_preemption_ipc  show each recv. preeption ipc ?
 *
 */
/*****************************************************************************/
l4_int32_t
VideoCoreComponentIntern_setAudioRTparams (l4_threadid_t thread_id,
					   unsigned long period,
					   unsigned long reservation_audio,
					   unsigned long reservation_video,
					   int verbose_preemption_ipc)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_setAudioRTparams_call
    (&thread_id, period, reservation_audio, reservation_video,
     verbose_preemption_ipc, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_setAudioRTparams failed (ret %d, exc %d)",
       ret, env.major);
    return -1;
  }
  return 0;
}
