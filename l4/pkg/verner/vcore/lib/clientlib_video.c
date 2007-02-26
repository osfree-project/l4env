/*
 * \brief   Clientlib for VERNER's video core component
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
#include <l4/vcore/functions_video.h>
#include "vcore_video-client.h"




/*****************************************************************************/
/**
 * \brief Callback function to connect outgoing stream. 
 * 
 * \param comp dsi_component_t
 * \param remote remote socket reference
 *
 * Calls VideoCoreComponentIntern_start_UncompressedVideoIn(), which unlocks
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
  ret = VideoCoreComponentIntern_start_UncompressedVideoOut_call
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
 * This function calls VideoCoreComponentIntern_disconnect_UncompressedVideoOut.
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
    VideoCoreComponentIntern_disconnect_UncompressedVideoOut (thread_id, 1);

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
VideoCoreComponentIntern_connect_UncompressedVideoOut (l4_threadid_t
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
  ret = VideoCoreComponentIntern_connect_UncompressedVideoOut_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_connect_UncompressedVideoOut failed (ret %d, exc %d)",
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
VideoCoreComponentIntern_disconnect_UncompressedVideoOut (l4_threadid_t
							  thread_id,
							  const l4_int32_t
							  close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_disconnect_UncompressedVideoOut_call
    (&thread_id, close_socket_flag, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_disconnect_UncompressedVideoOut failed (ret %d, exc %d)",
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
 * Calls VideoCoreComponentIntern_start_CompressedVideoIn(), which unlocks
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
  ret = VideoCoreComponentIntern_start_CompressedVideoIn_call
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
 * This function calls VideoCoreComponentIntern_disconnect_CompressedVideoIn.
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
  ret = VideoCoreComponentIntern_disconnect_CompressedVideoIn (thread_id, 1);

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
VideoCoreComponentIntern_connect_CompressedVideoIn (l4_threadid_t thread_id,
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
  ret = VideoCoreComponentIntern_connect_CompressedVideoIn_call
    (&thread_id, ctrl_ds, data_ds, &socket_ref, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_connect_CompressedVideoIn failed (ret %d, exc %d)",
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
VideoCoreComponentIntern_disconnect_CompressedVideoIn (l4_threadid_t
						       thread_id,
						       const l4_int32_t
						       close_socket_flag)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_disconnect_CompressedVideoIn_call
    (&thread_id, close_socket_flag, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_disconnect_CompressedVideoIn failed (ret %d, exc %d)",
       ret, env.major);
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
 * \param reservation_audio  (unused in video core)
 * \param reservation_video  reservation time for ONE video frame [microsecond]
 * \param verbose_preemption_ipc  show each recv. preeption ipc ?
 *
 */
/*****************************************************************************/
l4_int32_t
VideoCoreComponentIntern_setVideoRTparams (l4_threadid_t thread_id,
					   unsigned long period,
					   unsigned long reservation_audio,
					   unsigned long reservation_video,
					   int verbose_preemption_ipc)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_setVideoRTparams_call
    (&thread_id, period, reservation_audio, reservation_video,
     verbose_preemption_ipc, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_setVideoRTparams failed (ret %d, exc %d)",
       ret, env.major);
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set parameter for postprocessing
 * 
 * \param command Command to send to the postprocessing engine
 * \param ppName  Name of the postprocessing code
 * \param ppOptions Command to be send to the specific postprocessing code
 *
 * Using for example in this way:
 *  setVideoPostprocessing("add","libpostproc","h1:a,v1:a\0" to add the easiest 
 *  filter to processing-chain.
 *  Now add another processing option if wanted (add order is filter order!).
 *  setVideoPostprocessing("delete","libpostproc","h1:a,v1:a\0") deletes the filter.
 *  or
 *  setVideoPostprocessing("add","default","1") or  "2" or "3" adds one of the 
 *  predefined default deblocking(1,2,3) and deringing(3) filters to the processing-chain.
 *  and
 *  setVideoPostprocessing("activate","","") activates the processing-chain.
 *  setVideoPostprocessing("deactivate","","") deactivates the processing-chain.
 */
/*****************************************************************************/
l4_int32_t
VideoCoreComponentIntern_setVideoPostprocessing (l4_threadid_t thread_id,
						 const char *command,
						 const char *ppName,
						 const char *ppOptions)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_setVideoPostprocessing_call
    (&thread_id, command, ppName, ppOptions, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
  {
    LOG_Error
      ("VideoCoreComponentIntern_setVideoPostprocessing failed (ret %d, exc %d)",
       ret, env.major);
    return -1;
  }
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set parameter for QAP
 * 
 * \param useQAP   enable or disable qap - disabled means manuall setting of quality level
 * \param usePPasQAP enables usage of libpostprocess as QAP-levels while decoding
 * \param setQLevel  setting quality level (0..MAX)
 * \param currentQLevel returns current level
 * \param maxQLevel  returns highest supported quality level (0...MAX)
 *
 * If useQAP, usePPasQAP or setQLevel <0 it's just ignored as command, for instance 
 * usefull to get currentQLevel or maxQLevel without changing anything.
 * Attention: maxQLevel might change while working!!!!
 */
/*****************************************************************************/
l4_int32_t
VideoCoreComponentIntern_changeQAPSettings (l4_threadid_t thread_id,
					    l4_int32_t useQAP,
					    l4_int32_t usePPasQAP,
					    l4_int32_t setQLevel,
					    l4_int32_t * currentQLevel,
					    l4_int32_t * maxQLevel)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_changeQAPSettings_call
    (&thread_id, useQAP, usePPasQAP, setQLevel, currentQLevel, maxQLevel,
     &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    return -1;
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set filenames for learning and predicting decoding times
 * 
 * \param learnFile  the file to store newly learned decoding time prediction data
 * \param predictFile  the file to get previously gathered decoding time prediction data
 *
 * Either value can be the emtpy string, which disables learning/prediction respectively.
 */
/*****************************************************************************/
l4_int32_t
VideoCoreComponentIntern_setPrediction (l4_threadid_t thread_id,
					const char *learnFile,
					const char *predictFile)
{
  CORBA_Environment env = dice_default_environment;
  int ret = VideoCoreComponentIntern_setPrediction_call
    (&thread_id, learnFile, predictFile, &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    return -1;
  return 0;
}
