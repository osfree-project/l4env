/*
 * \brief   IPC server for VERNER's core component (audio pt.)
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
#include "vcore_audio-server.h"
#include "types.h"
#include "sender.h"
#include "receiver.h"
#include "arch_globals.h"
#include "audio_request_server.h"
#include "functions_audio.h"

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
static control_struct_t *audio_control = NULL;

#if PREDICT_DECODING_TIME
static prediction_context_t predict = {
  NULL, NULL, NULL
};
#endif

/*
 * the semaphore to control access to this struct
 * because it can be changed from too many threads 
 */
l4semaphore_t audio_control_access = L4SEMAPHORE_UNLOCKED;

#if BUILD_RT_SUPPORT
/* 
 * struct to remember user settings across connections
 */
typedef struct
{
  /* the lenght of one period in microsec */
  unsigned long rt_period;
  /* reservation time */
  unsigned long rt_reservation;
  /* verbose preemption-ipc */
  int rt_verbose_pipc;
} user_settings_t;
static user_settings_t user_settings;
#endif

/*****************************************************************************/
/**
* \brief Thread which handles DICE event loop
* 
*/
/*****************************************************************************/
int
VideoCoreComponent_Audio_dice_thread (void)
{

  /* register at nameserver - if it fails - we just add the appendix "-" */
  instance_no = register_instance (VCORE_AUDIO_NAME);
  if (instance_no < 0)
  {
    LOG_Error ("Failed to register instance. Exiting.");
    return instance_no;
  }
  else
    LOGdL (DEBUG_MINSTANCES, "registered as \"%s%i\" at namesserver.",
	   VCORE_AUDIO_NAME, instance_no);


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
l4_int32_t
  VideoCoreComponentIntern_connect_UncompressedAudioOut_component
  (CORBA_Object _dice_corba_obj, const l4dm_dataspace_t * ctrl_ds,
   const l4dm_dataspace_t * data_ds, dsi_socket_ref_t * socketref,
   CORBA_Server_Environment * _dice_corba_env)
{
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&audio_control_access);

  if (audio_control == NULL)
  {
    audio_control = malloc (sizeof (control_struct_t));
    if (!audio_control)
    {
      PANIC ("malloc failed!");
      /* unlock */
      l4semaphore_up (&audio_control_access);
      return -L4_ENOMEM;
    }
    /* set defaults */
    audio_control->stream_type = STREAM_TYPE_AUDIO;
    audio_control->work_thread = -1;
    audio_control->running_sem = L4SEMAPHORE_UNLOCKED;
#if BUILD_RT_SUPPORT
    audio_control->running_preempt = L4SEMAPHORE_UNLOCKED;	/* semaphore if the preempter thread is running */
    audio_control->rt_verbose_pipc = user_settings.rt_verbose_pipc;
    audio_control->rt_period = user_settings.rt_period;
    audio_control->rt_reservation = user_settings.rt_reservation;
#endif
    audio_control->create_sem = L4SEMAPHORE_UNLOCKED;
    audio_control->start_send_sem = L4SEMAPHORE_LOCKED;
    audio_control->start_recv_sem = L4SEMAPHORE_LOCKED;
    audio_control->send_in_use = 0;
    audio_control->recv_in_use = 0;
    /* codec defaults */
    /* we're decoding or passing */
    audio_control->plugin_ctrl.mode = PLUG_MODE_DEC;
    audio_control->plugin_ctrl.target_format = AUD_FMT_PCM;
#if PREDICT_DECODING_TIME
    audio_control->predict = &predict;
#endif
  }

  if (audio_control->send_in_use)
  {
    LOGL ("VCore: video connection already opened!");
    /* unlock */
    l4semaphore_up (&audio_control_access);
    return -1;
  }
  audio_control->send_in_use = 1;


  /* unlock */
  l4semaphore_up (&audio_control_access);

  return sender_open_socket (audio_control, ctrl_ds, data_ds, socketref);
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
l4_int32_t
VideoCoreComponentIntern_start_UncompressedAudioOut_component (CORBA_Object
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
  if ((!audio_control) || (!audio_control->send_in_use))
  {
    LOGL ("VCore: video connection not opened!");
    return -1;
  }
  return sender_start (audio_control, (dsi_socket_ref_t *) local,
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
l4_int32_t
  VideoCoreComponentIntern_disconnect_UncompressedAudioOut_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  int ret = 0;

  /* check if we have already a video sender connected */
  if ((!audio_control) || (!audio_control->send_in_use))
  {
    LOGL ("VCore: video connection not opened!");
    return -1;
  }
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&audio_control_access);

  ret = sender_close (audio_control, close_socket_flag);

  /* free use indicator */
  if (close_socket_flag)
  {
    audio_control->send_in_use = 0;
    /* if video_ctrl isn't used anymore - we free it */
    if ((!audio_control->send_in_use) && (!audio_control->recv_in_use))
    {
      free (audio_control);
      audio_control = NULL;
    }
  }

  /* unlock */
  l4semaphore_up (&audio_control_access);

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
l4_int32_t
VideoCoreComponentIntern_connect_CompressedAudioIn_component (CORBA_Object
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
  l4semaphore_down (&audio_control_access);

  if (audio_control == NULL)
  {
    audio_control = malloc (sizeof (control_struct_t));
    if (!audio_control)
    {
      PANIC ("malloc failed!");
      /* unlock */
      l4semaphore_up (&audio_control_access);
      return -L4_ENOMEM;
    }
    /* set defaults */
    audio_control->stream_type = STREAM_TYPE_AUDIO;
    audio_control->work_thread = -1;
    audio_control->running_sem = L4SEMAPHORE_UNLOCKED;
#if BUILD_RT_SUPPORT
    audio_control->running_preempt = L4SEMAPHORE_UNLOCKED;	/* semaphore if the preempter thread is running */
    audio_control->rt_verbose_pipc = user_settings.rt_verbose_pipc;
    audio_control->rt_period = user_settings.rt_period;
    audio_control->rt_reservation = user_settings.rt_reservation;
#endif
    audio_control->create_sem = L4SEMAPHORE_UNLOCKED;
    audio_control->start_send_sem = L4SEMAPHORE_LOCKED;
    audio_control->start_recv_sem = L4SEMAPHORE_LOCKED;
    audio_control->send_in_use = 0;
    audio_control->recv_in_use = 0;
    /* codec defaults */
    /* we're decoding or passing */
    audio_control->plugin_ctrl.mode = PLUG_MODE_DEC;
    audio_control->plugin_ctrl.target_format = AUD_FMT_PCM;
#if PREDICT_DECODING_TIME
    audio_control->predict = &predict;
#endif
  }

  if (audio_control->recv_in_use)
  {
    LOGL ("VCore: receiving video connection already opened!");
    /* unlock */
    l4semaphore_up (&audio_control_access);
    return -1;
  }
  audio_control->recv_in_use = 1;

  /* unlock */
  l4semaphore_up (&audio_control_access);

  return receiver_open_socket (audio_control, ctrl_ds, data_ds, socketref);
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
l4_int32_t
VideoCoreComponentIntern_start_CompressedAudioIn_component (CORBA_Object
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
  if ((!audio_control) || (!audio_control->recv_in_use))
  {
    LOGL ("VCore: receiving video connection not opened!");
    return -1;
  }
  return receiver_start (audio_control, (dsi_socket_ref_t *) local,
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
l4_int32_t
  VideoCoreComponentIntern_disconnect_CompressedAudioIn_component
  (CORBA_Object _dice_corba_obj, l4_int32_t close_socket_flag,
   CORBA_Server_Environment * _dice_corba_env)
{
  int ret = 0;

  /* check if we have already a video sender connected */
  if ((!audio_control) || (!audio_control->recv_in_use))
  {
    LOGL ("VCore: receiving video connection not opened!");
    return -1;
  }
  /* 
   * NOTE: both connect functions are called synchron by one app. 
   * So no mutex is required here. BUT just to be sure !
   */
  l4semaphore_down (&audio_control_access);

  ret = receiver_close (audio_control, close_socket_flag);

  /* free use indicator */
  if (close_socket_flag)
  {
    audio_control->recv_in_use = 0;
    /* if video_ctrl isn't used anymore - we free it */
    if ((!audio_control->send_in_use) && (!audio_control->recv_in_use))
    {
      free (audio_control);
      audio_control = NULL;
    }
  }

  /* unlock */
  l4semaphore_up (&audio_control_access);

  return ret;
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
VideoCoreComponentIntern_setAudioRTparams_component (CORBA_Object
						     _dice_corba_obj,
						     l4_uint32_t period,
						     l4_uint32_t
						     reservation_audio,
						     l4_uint32_t
						     reservation_video,
						     l4_int32_t
						     verbose_preemption_ipc,
						     CORBA_Server_Environment
						     * _dice_corba_env)
{
#if BUILD_RT_SUPPORT
  /* lock */
  l4semaphore_down (&audio_control_access);
  /* set current */
  if (audio_control)
  {
    audio_control->rt_verbose_pipc = (int) verbose_preemption_ipc;
    audio_control->rt_period = period;
    audio_control->rt_reservation = reservation_audio;
  }
  /* set in user settings */
  user_settings.rt_verbose_pipc = (int) verbose_preemption_ipc;
  user_settings.rt_period = period;
  user_settings.rt_reservation = reservation_video;
  /* unlock */
  l4semaphore_up (&audio_control_access);
#endif
  return 0;
}
