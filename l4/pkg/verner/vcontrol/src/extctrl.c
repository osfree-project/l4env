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

/* verner components */
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include <l4/vsync/functions.h>
#include "arch_plugins.h"

/* configuration */
#include "verner_config.h"

/* local */
#include "helper.h"
extern connect_chain_t video_chain;
extern connect_chain_t audio_chain;
#include "defines.h"
#include "extctrl.h"


/*
 * init of extctrl window
 */
void
ec_init (void)
{
  /* five rows are already used here (found player-UI.dpe!) */
#if BUILD_RT_SUPPORT || BUILD_goom
  int row = 5;
#endif

  /* bind */
  vdope_bind (app_id, "main_btn_ec", "press", ec_callback, (void *) CMD_SHOW);
  vdope_bind (app_id, "extctrl_scale_quality", "slid", ec_callback,
	      (void *) CMD_QAP_CHANGE);
  vdope_bind (app_id, "extctrl_btn_qap", "press", ec_callback,
	      (void *) CMD_QAP_TOGGLE);
  vdope_bind (app_id, "extctrl_btn_ppsend", "press", ec_callback,
	      (void *) CMD_PP_SEND);
  vdope_bind (app_id, "extctrl_entry_pp", "commit", ec_callback,
	      (void *) CMD_PP_SEND);

#if BUILD_goom
  /* goom */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"audio only stream visualisation:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_btn_goom=new Button()");
  vdope_cmd (app_id, "extctrl_btn_goom.set(-text \"Goom\")");
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_btn_goom, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_bind (app_id, "extctrl_btn_goom", "press", ec_callback,
	      (void *) CMD_GOOM_TOGGLE);
#endif


#if BUILD_RT_SUPPORT
  /* rt support: period, reservation times */
  row++;
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id,
	     "c.set(-text \"timing parameters for components [microsec]\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"You must commit and restart playing!\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  /* period */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"common period:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtperiod=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtperiod.set(-text \"%lu\")",
	      gui_state.rt_period);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtperiod, -column 2 -row %i)",
	      row);
  /* reservation times ... */
  /* demux audio */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"reservation time demuxer audio:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtres_demux_audio=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtres_demux_audio.set(-text \"%lu\")",
	      gui_state.rt_reservation_demux_audio);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtres_demux_audio, -column 2 -row %i)",
	      row);
  /* demux video */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"reservation time demuxer video:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtres_demux_video=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtres_demux_video.set(-text \"%lu\")",
	      gui_state.rt_reservation_demux_video);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtres_demux_video, -column 2 -row %i)",
	      row);
  /* core audio */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"reservation time sync audio:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtres_core_audio=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtres_core_audio.set(-text \"%lu\")",
	      gui_state.rt_reservation_core_audio);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtres_core_audio, -column 2 -row %i)",
	      row);
  /* core video */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"reservation time sync video:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtres_core_video=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtres_core_video.set(-text \"%lu\")",
	      gui_state.rt_reservation_core_video);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtres_core_video, -column 2 -row %i)",
	      row);
  /* sync audio */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"reservation time sync audio:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtres_sync_audio=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtres_sync_audio.set(-text \"%lu\")",
	      gui_state.rt_reservation_sync_audio);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtres_sync_audio, -column 2 -row %i)",
	      row);
  /* sync video */
  vdope_cmd (app_id, "c=new Label()");
  vdope_cmd (app_id, "c.set(-text \"reservation time sync video:\")");
  vdope_cmdf (app_id, "extctrl_grid.place(c, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_cmd (app_id, "extctrl_entry_rtres_sync_video=new Entry()");
  vdope_cmdf (app_id, "extctrl_entry_rtres_sync_video.set(-text \"%lu\")",
	      gui_state.rt_reservation_sync_video);
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_entry_rtres_sync_video, -column 2 -row %i)",
	      row);
  /* commit button */
  vdope_cmd (app_id, "extctrl_btn_rttimes=new Button()");
  vdope_cmd (app_id, "extctrl_btn_rttimes.set(-text \"Commit\")");
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_btn_rttimes, -column 2 -row %i -align \"w\")",
	      ++row);
  vdope_bind (app_id, "extctrl_btn_rttimes", "press", ec_callback,
	      (void *) CMD_RT_TIMES_UPDATE);
  /* verbose preemption ipcs in demux */
  vdope_cmd (app_id, "extctrl_btn_rt_verbose_demux=new Button()");
  vdope_cmd (app_id,
	     "extctrl_btn_rt_verbose_demux.set(-text \"Verbose P-IPCs in Demuxer \")");
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_btn_rt_verbose_demux, -column 1 -row %i -align \"w\")",
	      row);
  vdope_bind (app_id, "extctrl_btn_rt_verbose_demux", "press", ec_callback,
	      (void *) CMD_RT_VERBOSE_PIPC_TOGGLE_DEMUX);
  /* verbose preemption ipcs in audio core */
  vdope_cmd (app_id, "extctrl_btn_rt_verbose_core_audio=new Button()");
  vdope_cmd (app_id,
	     "extctrl_btn_rt_verbose_core_audio.set(-text \"Verbose P-IPCs in Audio-Core \")");
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_btn_rt_verbose_core_audio, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_bind (app_id, "extctrl_btn_rt_verbose_core_audio", "press",
	      ec_callback, (void *) CMD_RT_VERBOSE_PIPC_TOGGLE_CORE_AUDIO);
  /* verbose preemption ipcs in video core */
  vdope_cmd (app_id, "extctrl_btn_rt_verbose_core_video=new Button()");
  vdope_cmd (app_id,
	     "extctrl_btn_rt_verbose_core_video.set(-text \"Verbose P-IPCs in Video-Core \")");
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_btn_rt_verbose_core_video, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_bind (app_id, "extctrl_btn_rt_verbose_core_video", "press",
	      ec_callback, (void *) CMD_RT_VERBOSE_PIPC_TOGGLE_CORE_VIDEO);
  /* verbose preemption ipcs in sync */
  vdope_cmd (app_id, "extctrl_btn_rt_verbose_sync=new Button()");
  vdope_cmd (app_id,
	     "extctrl_btn_rt_verbose_sync.set(-text \"Verbose P-IPCs in Sync \")");
  vdope_cmdf (app_id,
	      "extctrl_grid.place(extctrl_btn_rt_verbose_sync, -column 1 -row %i -align \"w\")",
	      ++row);
  vdope_bind (app_id, "extctrl_btn_rt_verbose_sync", "press", ec_callback,
	      (void *) CMD_RT_VERBOSE_PIPC_TOGGLE_SYNC);

#endif


}


/*
 * callback extra controls' window
 */
void
ec_callback (dope_event * e, void *arg)
{
  /* is playlist window visible ? */
  static int visible = 0;
#if BUILD_RT_SUPPORT
  long result_long;
#endif
  char result[256];
  int cmd = (int) arg;
  switch (cmd)
  {
#if BUILD_goom
  case CMD_GOOM_TOGGLE:
    if (gui_state.fx_plugin != FX_PLUG_ID_GOOM)
    {
      /* enable goom */
      VideoSyncComponentIntern_setFxPlugin
	(video_chain.vsync_thread_id, FX_PLUG_ID_GOOM);
      vdope_cmdf (app_id, "extctrl_btn_goom.set(-state 1)");
      gui_state.fx_plugin = FX_PLUG_ID_GOOM;
    }
    else
    {
      /* disable goom and all other plugins! */
      VideoSyncComponentIntern_setFxPlugin
	(video_chain.vsync_thread_id, FX_PLUG_ID_NONE);
      vdope_cmdf (app_id, "extctrl_btn_goom.set(-state 0)");
      gui_state.fx_plugin = FX_PLUG_ID_NONE;
    }

    break;
#endif
#if BUILD_RT_SUPPORT
  case CMD_RT_TIMES_UPDATE:
    /* 
     * set period to new value for next stream to play 
     * will be set while connecting all components 
     */
    vdope_req (app_id, result, 256, "extctrl_entry_rtperiod.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error ("invalid value for period! Set to default.");
      gui_state.rt_period = RT_DEFAULT_PERIOD;
    }
    else
      gui_state.rt_period = (unsigned long) result_long;

    /* set reservation times */
    /* demuxer audio */
    vdope_req (app_id, result, 256, "extctrl_entry_rtres_demux_audio.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error
	("invalid value for demuxer-audio's reservation time! Set to default.");
      gui_state.rt_reservation_demux_audio = RT_DEMUXER_AUDIO_EXEC_TIME;
    }
    else
      gui_state.rt_reservation_demux_audio = (unsigned long) result_long;
    /* demuxer video */
    vdope_req (app_id, result, 256, "extctrl_entry_rtres_demux_video.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error
	("invalid value for demuxer-video's reservation time! Set to default.");
      gui_state.rt_reservation_demux_video = RT_DEMUXER_VIDEO_EXEC_TIME;
    }
    else
      gui_state.rt_reservation_demux_video = (unsigned long) result_long;
    /* core audio */
    vdope_req (app_id, result, 256, "extctrl_entry_rtres_core_audio.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error
	("invalid value for core-audio's reservation time! Set to default.");
      gui_state.rt_reservation_core_audio = RT_CORE_AUDIO_EXEC_TIME;
    }
    else
      gui_state.rt_reservation_core_audio = (unsigned long) result_long;
    /* core video */
    vdope_req (app_id, result, 256, "extctrl_entry_rtres_core_video.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error
	("invalid value for core-video's reservation time! Set to default.");
      gui_state.rt_reservation_core_video = RT_CORE_VIDEO_EXEC_TIME;
    }
    else
      gui_state.rt_reservation_core_video = (unsigned long) result_long;
    /* sync audio */
    vdope_req (app_id, result, 256, "extctrl_entry_rtres_sync_audio.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error
	("invalid value for sync-audio's reservation time! Set to default.");
      gui_state.rt_reservation_sync_audio = RT_SYNC_AUDIO_EXEC_TIME;
    }
    else
      gui_state.rt_reservation_sync_audio = (unsigned long) result_long;
    /* sync video */
    vdope_req (app_id, result, 256, "extctrl_entry_rtres_sync_video.text()");
    result_long = atol (result);
    if (result_long <= 0)
    {
      LOG_Error
	("invalid value for sync-video's reservation time! Set to default.");
      gui_state.rt_reservation_sync_video = RT_SYNC_VIDEO_EXEC_TIME;
    }
    else
      gui_state.rt_reservation_sync_video = (unsigned long) result_long;

    /* update gui */
    vdope_cmdf (app_id, "extctrl_entry_rtres_demux_audio.set(-text \"%lu\")",
		gui_state.rt_reservation_demux_audio);
    vdope_cmdf (app_id, "extctrl_entry_rtres_demux_video.set(-text \"%lu\")",
		gui_state.rt_reservation_demux_video);
    vdope_cmdf (app_id, "extctrl_entry_rtres_core_audio.set(-text \"%lu\")",
		gui_state.rt_reservation_core_audio);
    vdope_cmdf (app_id, "extctrl_entry_rtres_core_video.set(-text \"%lu\")",
		gui_state.rt_reservation_core_video);
    vdope_cmdf (app_id, "extctrl_entry_rtres_sync_audio.set(-text \"%lu\")",
		gui_state.rt_reservation_sync_audio);
    vdope_cmdf (app_id, "extctrl_entry_rtres_sync_video.set(-text \"%lu\")",
		gui_state.rt_reservation_sync_video);

    /* eof RT_TIMES_UPDATE: break switch command */
    break;

    /* toggle preemption ipc verbose */
  case CMD_RT_VERBOSE_PIPC_TOGGLE_DEMUX:
    gui_state.rt_verbose_preemption_ipc_demux ^= 1;
    vdope_cmdf (app_id, "extctrl_btn_rt_verbose_demux.set(-state %i)",
		gui_state.rt_verbose_preemption_ipc_demux);
    break;
  case CMD_RT_VERBOSE_PIPC_TOGGLE_CORE_AUDIO:
    gui_state.rt_verbose_preemption_ipc_core_audio ^= 1;
    vdope_cmdf (app_id, "extctrl_btn_rt_verbose_core_audio.set(-state %i)",
		gui_state.rt_verbose_preemption_ipc_core_audio);
    break;
  case CMD_RT_VERBOSE_PIPC_TOGGLE_CORE_VIDEO:
    gui_state.rt_verbose_preemption_ipc_core_video ^= 1;
    vdope_cmdf (app_id, "extctrl_btn_rt_verbose_core_video.set(-state %i)",
		gui_state.rt_verbose_preemption_ipc_core_video);
    break;
  case CMD_RT_VERBOSE_PIPC_TOGGLE_SYNC:
    gui_state.rt_verbose_preemption_ipc_sync ^= 1;
    vdope_cmdf (app_id, "extctrl_btn_rt_verbose_sync.set(-state %i)",
		gui_state.rt_verbose_preemption_ipc_sync);
    break;
#endif

  case CMD_SHOW:
    /* toggle show */
    if (!visible)
    {
      /* show */
      vdope_cmdf (app_id, "main_btn_ec.set(-state 1)");
      vdope_cmd (app_id, "extctrl_win.open()");
      vdope_cmd (app_id, "extctrl_win.top()");
      visible = 1;
    }
    else
    {
      /* hide */
      vdope_cmdf (app_id, "main_btn_ec.set(-state 0)");
      vdope_cmd (app_id, "extctrl_win.close()");
      visible = 0;
    }
    break;

  case CMD_QAP_CHANGE:
    /* QAP scaler */
    {
      int qual;
      int dummy;
      char *filterno[3] = { "1", "2", "3" }; // string constants are zero terminated by default
      vdope_req (app_id, result, 16, "extctrl_scale_quality.value");
      qual = (int) strtod (result, (char **) NULL);
      vdope_cmdf (app_id, "extctrl_scale_quality.set(-value %i)", qual);
      video_chain.QAP_on = 0;
      vdope_cmdf (app_id, "extctrl_btn_qap.set(-state %i)",
		  video_chain.QAP_on);
      /* disable QAP in core */
      VideoCoreComponentIntern_changeQAPSettings (video_chain.vcore_thread_id,
						  video_chain.QAP_on, 0, qual,
						  &dummy, &dummy);
      /* if we set it manullay we use the deault filters */
      if ((qual > 0) && (qual < 4))
      {
	VideoCoreComponentIntern_setVideoPostprocessing
	  (video_chain.vcore_thread_id, "add", "default", filterno[qual - 1]);
	VideoCoreComponentIntern_setVideoPostprocessing
	  (video_chain.vcore_thread_id, "activate", "", "");
      }
      else if (qual == 0)
      {
	/* remove defaule filter from chain */
	VideoCoreComponentIntern_setVideoPostprocessing
	  (video_chain.vcore_thread_id, "delete", "default", "");
	VideoCoreComponentIntern_setVideoPostprocessing
	  (video_chain.vcore_thread_id, "deactivate", "", "");
      }
    }
    break;

  case CMD_QAP_TOGGLE:
    /* QAP button */
    {
      int dummy;

      video_chain.QAP_on ^= 1;
      vdope_cmdf (app_id, "extctrl_btn_qap.set(-state %i)",
		  video_chain.QAP_on);
      /* change QAP settings in core */
      VideoCoreComponentIntern_changeQAPSettings (video_chain.vcore_thread_id,
						  video_chain.QAP_on, -1, -1,
						  &dummy, &dummy);
    }
    break;

  case CMD_PP_SEND:
    /* Send postprocessing command */
    vdope_req (app_id, result, 256, "extctrl_entry_pp.text()");
    /* clr text */
    vdope_cmd (app_id, "extctrl_entry_pp.set(-text \"\")");

    /* parse */
    if (strlen (result) > 0)
    {
      char *command = result;
      char *ppName = strchr (command, ' ');
      char *ppOptions = NULL;

      if (ppName)
      {
	/* set \0 and end of command */
	ppName[0] = '\0';
	ppName++;
	ppOptions = strchr (ppName, ' ');
	/* set \0 and end of ppName */
	if (ppOptions)
	{
	  ppOptions[0] = '\0';
	  ppOptions++;
	}
      }
      VideoCoreComponentIntern_setVideoPostprocessing
	(video_chain.vcore_thread_id, command, ppName ? ppName : "",
	 ppOptions ? ppOptions : "");
    }
    break;

  }				/* end case */
}
