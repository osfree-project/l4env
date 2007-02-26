/*
 * \brief   IPC server for VERNER's core component (video pt.)
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
#include <stdlib.h>		/* malloc, free */
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* verner */
#include "arch_globals.h"
#include "arch_plugins.h"

/* local */
#include "vcore_video-server.h"
#include "types.h"
#include "sender.h"
#include "receiver.h"
#include "arch_globals.h"
#include "video_request_server.h"
#include "functions_video.h"
#include "postproc.h"

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
static control_struct_t *video_control = NULL;

#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
static prediction_context_t predict = {
#if PREDICT_DECODING_TIME
  NULL, NULL, NULL
#endif
#if H264_SLICE_SCHEDULE
  NULL, NULL
#endif
};
#endif

#if H264_SLICE_SCHEDULE
void h264_machine_speed(float speed);
#endif

/*
 * the semaphore to control access to this struct
 * because it can be changed from too many threads
 */
l4semaphore_t video_control_access = L4SEMAPHORE_UNLOCKED;

/*
 * struct to remember user settings across connections
 */
typedef struct
{
  int dummy;			/* dummy to compile without QAP and RT_SUPPORT */
#if VCORE_VIDEO_ENABLE_QAP
  /* QAP settings */
  qap_struct_t qap_settings;
#endif
#if BUILD_RT_SUPPORT
  /* the lenght of one period in microsec */
  unsigned long rt_period;
  /* reservation time */
  unsigned long rt_reservation;
  /* reservation time for optional part */
  unsigned long rt_reservation_opt;
  /* verbose preemption-ipc */
  int rt_verbose_pipc;
#endif
} user_settings_t;
static user_settings_t user_settings;


/*****************************************************************************/
/**
* \brief Thread which handles DICE event loop
* 
*/
/*****************************************************************************/
int
VideoCoreComponent_Video_dice_thread (void)
{

  /* register at nameserver - if it fails - we just add the appendix "-" */
  instance_no = register_instance (VCORE_VIDEO_NAME);
  if (instance_no < 0)
  {
    LOG_Error ("Failed to register instance. Exiting.");
    return instance_no;
  }
  else
    LOGdL (DEBUG_MINSTANCES, "registered as \"%s%i\" at namesserver.",
	   VCORE_VIDEO_NAME, instance_no);

  /* set user settings to default values */
  memset (&user_settings, 0, sizeof (user_settings_t));

  /* IDL server loop */
  VideoCoreComponentIntern_server_loop (NULL);

  Panic ("left server loop - this is really bad.");

  return 0;
}


/*****************************************************************************/
/**
* \brief Connnect VideoCore-Component
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param component setup dsi_component_t with callback functions and socket_ref
*
* Attach datascpaces to VideoCore-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
long
  VideoCoreComponentIntern_connect_UncompressedVideoOut_component
  (CORBA_Object _dice_corba_obj, const l4dm_dataspace_t * ctrl_ds,
   const l4dm_dataspace_t * data_ds, dsi_socket_ref_t * socketref,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&video_control_access);

  if (video_control == NULL)
  {
    video_control = malloc (sizeof (control_struct_t));
    if (!video_control)
    {
      PANIC ("malloc failed!");
      /* unlock */
      l4semaphore_up (&video_control_access);
      return -L4_ENOMEM;
    }
    /* set defaults */
    video_control->stream_type = STREAM_TYPE_VIDEO;
    video_control->work_thread = -1;
    video_control->running_sem = L4SEMAPHORE_UNLOCKED;
#if BUILD_RT_SUPPORT
    video_control->running_preempt = L4SEMAPHORE_UNLOCKED;	/* semaphore if the preempter thread is running */
    /* take user set values */
    video_control->rt_verbose_pipc = user_settings.rt_verbose_pipc;
    video_control->rt_period = user_settings.rt_period;
    video_control->rt_reservation = user_settings.rt_reservation;
    video_control->rt_preempt_count = 0;
#endif
    video_control->create_sem = L4SEMAPHORE_UNLOCKED;
    video_control->start_send_sem = L4SEMAPHORE_LOCKED;
    video_control->start_recv_sem = L4SEMAPHORE_LOCKED;
    video_control->send_in_use = 0;
    video_control->recv_in_use = 0;
    /* codec defaults */
    /* we're decoding or passing */
    video_control->plugin_ctrl.mode = PLUG_MODE_DEC;
    video_control->plugin_ctrl.target_format = VID_FMT_RAW;
    video_control->plugin_ctrl.target_colorspace = VID_YUV420;
#if VCORE_VIDEO_ENABLE_QAP
    /* user set qap settings */
    video_control->qap_settings.useQAP = user_settings.qap_settings.useQAP;
    video_control->qap_settings.usePPasQAP =
      user_settings.qap_settings.usePPasQAP;
#endif
    video_control->plugin_ctrl.maxQLevel =
      video_control->plugin_ctrl.minQLevel =
      video_control->plugin_ctrl.currentQLevel = 0;
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
    video_control->predict = &predict;
#endif
  }

  if (video_control->send_in_use)
  {
    LOGL ("VCore: video connection already opened!");
    /* unlock */
    l4semaphore_up (&video_control_access);
    return -1;
  }
  video_control->send_in_use = 1;


  /* unlock */
  l4semaphore_up (&video_control_access);

  return sender_open_socket (video_control, ctrl_ds, data_ds, socketref);
}

/*****************************************************************************/
/**
* \brief Start VideoCore-Component's work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* unlocks a mutex and let the work_thread run. */
/*****************************************************************************/
long
VideoCoreComponentIntern_start_UncompressedVideoOut_component (CORBA_Object
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
  if ((!video_control) || (!video_control->send_in_use))
  {
    LOGL ("VCore: video connection not opened!");
    return -1;
  }
  return sender_start (video_control, (dsi_socket_ref_t *) local,
		       (dsi_socket_ref_t *) remote);
}


/*****************************************************************************/
/**
 * \brief Disconnect VideoCore-Component
 * 
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 */
/*****************************************************************************/
long
  VideoCoreComponentIntern_disconnect_UncompressedVideoOut_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  int ret = 0;

  /* check if we have already a video sender connected */
  if ((!video_control) || (!video_control->send_in_use))
  {
    LOGL ("VCore: video connection not opened!");
    return -1;
  }
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&video_control_access);

  ret = sender_close (video_control, close_socket_flag);

  /* free use indicator */
  if (close_socket_flag)
  {
    video_control->send_in_use = 0;
    /* if video_ctrl isn't used anymore - we free it */
    if ((!video_control->send_in_use) && (!video_control->recv_in_use))
    {
      free (video_control);
      video_control = NULL;
    }
  }

  /* unlock */
  l4semaphore_up (&video_control_access);

  return ret;

}



/*****************************************************************************/
/**
* \brief Connnect VideoCore-Component
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param component setup dsi_component_t with callback functions and socket_ref
*
* Attach datascpaces to VideoCore-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
long
VideoCoreComponentIntern_connect_CompressedVideoIn_component (CORBA_Object
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
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&video_control_access);

  if (video_control == NULL)
  {
    video_control = malloc (sizeof (control_struct_t));
    if (!video_control)
    {
      PANIC ("malloc failed!");
      /* unlock */
      l4semaphore_up (&video_control_access);
      return -L4_ENOMEM;
    }
    /* set defaults */
    video_control->stream_type = STREAM_TYPE_VIDEO;
    video_control->work_thread = -1;
    video_control->running_sem = L4SEMAPHORE_UNLOCKED;
#if BUILD_RT_SUPPORT
    video_control->running_preempt = L4SEMAPHORE_UNLOCKED;	/* semaphore if the preempter thread is running */
    /* take user set values */
    video_control->rt_verbose_pipc = user_settings.rt_verbose_pipc;
    video_control->rt_period = user_settings.rt_period;
    video_control->rt_reservation = user_settings.rt_reservation;
    video_control->rt_preempt_count = 0;
#endif
    video_control->create_sem = L4SEMAPHORE_UNLOCKED;
    video_control->start_send_sem = L4SEMAPHORE_LOCKED;
    video_control->start_recv_sem = L4SEMAPHORE_LOCKED;
    video_control->send_in_use = 0;
    video_control->recv_in_use = 0;
    /* codec defaults */
    /* we're decoding or passing */
    video_control->plugin_ctrl.mode = PLUG_MODE_DEC;
    video_control->plugin_ctrl.target_format = VID_FMT_RAW;
    video_control->plugin_ctrl.target_colorspace = VID_YUV420;
#if VCORE_VIDEO_ENABLE_QAP
    /* user set qap settings */
    video_control->qap_settings.useQAP = user_settings.qap_settings.useQAP;
    video_control->qap_settings.usePPasQAP =
      user_settings.qap_settings.usePPasQAP;
#endif
    video_control->plugin_ctrl.maxQLevel =
      video_control->plugin_ctrl.minQLevel =
      video_control->plugin_ctrl.currentQLevel = 0;
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
    video_control->predict = &predict;
#endif
  }

  if (video_control->recv_in_use)
  {
    LOGL ("VCore: receiving video connection already opened!");
    /* unlock */
    l4semaphore_up (&video_control_access);
    return -1;
  }
  video_control->recv_in_use = 1;

  /* unlock */
  l4semaphore_up (&video_control_access);

  return receiver_open_socket (video_control, ctrl_ds, data_ds, socketref);
}

/*****************************************************************************/
/**
* \brief Start VideoCore-Component's work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* unlocks a mutex and let the work_thread run. */
/*****************************************************************************/
long
VideoCoreComponentIntern_start_CompressedVideoIn_component (CORBA_Object
							    _dice_corba_obj,
							    const
							    dsi_socket_ref_t
							    * local,
							    const
							    dsi_socket_ref_t
							    * remote,
							    CORBA_Server_Environment
							    * _dice_corba_env)
{
  /* check if we have already a video sender connected */
  if ((!video_control) || (!video_control->recv_in_use))
  {
    LOGL ("VCore: receiving video connection not opened!");
    return -1;
  }
  return receiver_start (video_control, (dsi_socket_ref_t *) local,
			 (dsi_socket_ref_t *) remote);
}


/*****************************************************************************/
/**
 * \brief Disconnect VideoCore-Component
 * 
 * This function ensures work_thread is closed and nobody is working 
 * with DSI-sockets. Afterwards the DSI-socket is closed.
 */
/*****************************************************************************/
long
  VideoCoreComponentIntern_disconnect_CompressedVideoIn_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  int ret = 0;

  /* check if we have already a video sender connected */
  if ((!video_control) || (!video_control->recv_in_use))
  {
    LOGL ("VCore: receiving video connection not opened!");
    return -1;
  }
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&video_control_access);

  ret = receiver_close (video_control, close_socket_flag);

  /* free use indicator */
  if (close_socket_flag)
  {
    video_control->recv_in_use = 0;
    /* if video_ctrl isn't used anymore - we free it */
    if ((!video_control->send_in_use) && (!video_control->recv_in_use))
    {
      free (video_control);
      video_control = NULL;
    }
  }

  /* unlock */
  l4semaphore_up (&video_control_access);

  return ret;
}


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
long
VideoCoreComponentIntern_setVideoRTparams_component (CORBA_Object _dice_corba_obj,
						     unsigned long period,
						     unsigned long reservation_audio,
						     unsigned long reservation_video,
						     int verbose_preemption_ipc,
						     CORBA_Server_Environment * _dice_corba_env)
{
#if BUILD_RT_SUPPORT
  /* lock */
  l4semaphore_down (&video_control_access);
  /* set current */
  if (video_control)
  {
    video_control->rt_verbose_pipc = (int) verbose_preemption_ipc;
    video_control->rt_period = period;
    video_control->rt_reservation = reservation_video;
    video_control->rt_reservation_opt = reservation_audio;
  }
  /* set in user settings */
  user_settings.rt_verbose_pipc = (int) verbose_preemption_ipc;
  user_settings.rt_period = period;
  user_settings.rt_reservation = reservation_video;
  user_settings.rt_reservation_opt = reservation_audio;
  /* unlock */
  l4semaphore_up (&video_control_access);
#endif
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
long
VideoCoreComponentIntern_setVideoPostprocessing_component (CORBA_Object
							   _dice_corba_obj,
							   const char
							   *command,
							   const char *ppName,
							   const char
							   *ppOptions,
							   CORBA_Server_Environment
							   * _dice_corba_env)
{
  return postProcessEngineCommand (command, ppName, ppOptions);
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
long
VideoCoreComponentIntern_changeQAPSettings_component (CORBA_Object
						      _dice_corba_obj,
						      l4_int32_t useQAP,
						      l4_int32_t usePPasQAP,
						      l4_int32_t setQLevel,
						      l4_int32_t *
						      currentQLevel,
						      l4_int32_t * maxQLevel,
						      CORBA_Server_Environment
						      * _dice_corba_env)
{
  /* lock */
  l4semaphore_down (&video_control_access);

  if (video_control == NULL)
  {
    /* unlock */
    l4semaphore_up (&video_control_access);
    //LOG_Error ("Not connected. Connect first, then change QAP settings !");
    /* just returning zero */
    *maxQLevel = *currentQLevel = 0;
    return -L4_ENOTSUPP;
  }

#if VCORE_VIDEO_ENABLE_QAP /*QAP*/
  /* setup wanted values */
  if (useQAP >= 0)
  {
    video_control->qap_settings.useQAP = useQAP;
    user_settings.qap_settings.useQAP = useQAP;
  }
#if VCORE_VIDEO_FORCE_LIBPOSTPROC
  /* force libpostprocess as QAP */
  video_control->qap_settings.usePPasQAP = usePPasQAP;
  user_settings.qap_settings.usePPasQAP = usePPasQAP;
#else
  /* should we use PP for QAP */
  if (usePPasQAP >= 0)
  {
    /* activate default filtering */
    video_control->qap_settings.usePPasQAP = usePPasQAP;
    user_settings.qap_settings.usePPasQAP = usePPasQAP;
    postProcessEngineCommand ("add", "default", "1");
  }
#endif
  if (setQLevel >= 0)
    video_control->plugin_ctrl.currentQLevel =
      setQLevel + video_control->plugin_ctrl.minQLevel;


  /* activate engine */
  if ((video_control->qap_settings.usePPasQAP > 0)
      || (video_control->plugin_ctrl.currentQLevel > 0))
    postProcessEngineCommand ("activate", "", "");


  /* return current values */
  *maxQLevel =
    (video_control->plugin_ctrl.maxQLevel -
     video_control->plugin_ctrl.minQLevel);
  *currentQLevel =
    (video_control->plugin_ctrl.currentQLevel -
     video_control->plugin_ctrl.minQLevel);

#else /* no QAP */

  /* we set only the current quality */
  if (setQLevel >= 0)
    video_control->plugin_ctrl.currentQLevel = setQLevel;
  /* return values */
  *maxQLevel = 0;
  *currentQLevel = video_control->plugin_ctrl.currentQLevel;

#endif

  /* unlock */
  l4semaphore_up (&video_control_access);
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set filenames for learning and predicting decoding times
 * 
 * \param learnFile  the file to store newly learned decoding time prediction data
 * \param predictFile  the file to get previously gathered decoding time prediction data
 *
 * Either value can be the empty string, which disables learning/prediction respectively.
 */
/*****************************************************************************/
long
VideoCoreComponentIntern_setPrediction_component (CORBA_Object _dice_corba_obj,
						  const char *learnFile,
						  const char *predictFile,
						  CORBA_Server_Environment *_dice_corba_env)
{
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
  /* lock */
  l4semaphore_down (&video_control_access);
  
  free(video_control->predict->learn_file);
  free(video_control->predict->predict_file);
  video_control->predict->learn_file   = strdup(learnFile);
  video_control->predict->predict_file = strdup(predictFile);

  /* unlock */
  l4semaphore_up (&video_control_access);
#endif
  return 0;
}


/*****************************************************************************/
/**
 * \brief Set simulated machine speed for H.264 slice scheduling
 * 
 * \param speed  the machine speed factor (in percent) to simulate
 */
/*****************************************************************************/
long
VideoCoreComponentIntern_setH264Speed_component (CORBA_Object _dice_corba_obj,
						  l4_int32_t speed,
						  CORBA_Server_Environment *_dice_corba_env)
{
#if H264_SLICE_SCHEDULE
  h264_machine_speed((float)speed / 100.0);
#endif
  return 0;
}
