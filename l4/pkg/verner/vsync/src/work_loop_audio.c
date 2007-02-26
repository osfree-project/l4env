/*
 * \brief   Receiver for VERNER's sync component
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

#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* local */
#include "work_loop.h"
#include "playerstatus.h"

/* configuration */
#include "verner_config.h"
#include "x86cpucaps.h"

/* verner */
#include "arch_globals.h"

/* plugins
 * Note: this should go into an seperate file if we have more 
 * than one export plugin per stream type 
 */
#include "arch_plugins.h"
#include "ao_oss.h"
#include "vo_dope.h"
#if VSYNC_BUILD_WITH_CON
#include "vo_con.h"
#include <l4/con/l4con.h>
#endif

#if BUILD_RT_SUPPORT
/* rt support */
#include <l4/sys/rt_sched.h> // RT Scheduling
#include <l4/rmgr/librmgr.h> // rmgr_set_prio
#include <l4/sigma0/kip.h> // l4sigma0_kip_map
#if RT_USE_CPU_RESERVE
#include <l4/cpu_reserve/sched.h>
#endif
#endif // RT support

#if BUILD_goom
/* goom */
#include "fx_goom.h"
#endif

#if VSYNC_AUDIO_BENCHMARK || RTMON_DSI_BENCHMARK
/* benchmark */
/* rt_mon stuff */
#include <l4/rt_mon/histogram.h>
#include <l4/rt_mon/event_list.h>
/* setup selected timer source for rt_mon */
#if RT_MON_TSC
#define RT_MON_TIMER_SOURCE RT_MON_TSC_TIME
#endif
#if RT_MON_PROCESS
#define RT_MON_TIMER_SOURCE RT_MON_THREAD_TIME
#endif
#endif /* end benchmark */

/*****************************************************************************/
/**
 * \brief Work thread.
 *
 * \param data Thread data (unused).
 */
/*****************************************************************************/
void
work_loop_audio (void *data)
{
  int ret;
  l4_uint32_t num;
  dsi_packet_t *p;
  void *addr;
  l4_size_t size;
  control_struct_t *control;
  int playmode;

  /* fx plugin */
  int current_fx_plugin_id = FX_PLUG_ID_NONE;

#if VSYNC_BUILD_WITH_CON
  /* con support */
  l4_threadid_t con_l4id = L4_INVALID_ID;
#endif

#if VSYNC_AUDIO_BENCHMARK
  rt_mon_histogram_t *hist = NULL;
#endif
#if RTMON_DSI_BENCHMARK
  rt_mon_event_list_t *list = NULL;
  rt_mon_basic_event_t be;
#endif

#if BUILD_RT_SUPPORT
  /* rt support */
  l4_kernel_info_t *kinfo = 0;
  int mand_time, mand_prio;
#if RT_USE_CPU_RESERVE
  int mand_id;
#endif
  /* the lenght of one period in µsec */
  unsigned long ul_usec_period;
  /* the calculated reservation time */
  unsigned long ul_usec_reservation;
  /* how many packets(frames/chunks) we have to process in one period */
  int packets_per_period;
  /* noof packets we have still to process in current loop */
  int packets_per_period_to_do;
#endif

  /*
   * return value for dsi packet_get_* - ensure we received all(!) packets
   * so sender isn't deadlocked
   */
  int recv_return = 0;

#if DEBUG_DS
  /* dsi_fill-level */
  int dsi_filllevel;
#endif

  /* get ptr to control structure */
  control = (control_struct_t *) data;

  /* set funct-ptrs to export */
  control->out_init = ao_oss_init;
  control->out_commit = ao_oss_commit;
  control->out_step = ao_oss_step;
  control->out_close = ao_oss_close;
  control->out_getPosition = ao_oss_getPosition;
  control->out_setVolume = ao_oss_setVolume;

  /* indicate we're running */
  l4semaphore_down (&control->running_sem);

  /* wait for start event */
  l4semaphore_down (&control->start_sem);
  LOGdL (DEBUG_THREAD, "work thread started");

  /* should we stop working before doing anything ? */
  if (control->shutdown_work_thread)
    goto shutdown_recv_thread;

#if BUILD_RT_SUPPORT
  /* enable blocking mode for first packet */
  control->socket->flags |= DSI_SOCKET_BLOCK;
#endif
  /* get first packet */
  recv_return = dsi_packet_get (control->socket, &p);
  if (recv_return)
  {
    if (recv_return == -DSI_ENOPACKET)
    {
      LOGL ("dsi_packet_get returned DSI_ENOPACKET");
      goto shutdown_recv_thread;
    }
    else
    {
      Panic ("dsi_packet_get failed (%x)", recv_return);
      goto shutdown_recv_thread;
    }
  }
#if BUILD_RT_SUPPORT
  /* disable blocking mode */
  control->socket->flags &= ~DSI_SOCKET_BLOCK;
#endif
  
  /* get packet number */
  recv_return = dsi_packet_get_no (control->socket, p, &num);
  if (recv_return)
  {
    Panic ("dsi_packet_get_no number failed (%d)", recv_return);
    goto shutdown_recv_thread;
  }

  /* get packet data */
  recv_return = dsi_packet_get_data (control->socket, p, &addr, &size);
  if (recv_return)
  {
    if (recv_return == -DSI_EEOS)
    {
      LOGL ("dsi_packet_get_data returned DSI_EEOS");
      goto shutdown_recv_thread;
    }
    else
    {
      Panic ("dsi_packet_get_data failed (%d)", recv_return);
      goto shutdown_recv_thread;
    }
  }

  /* detecting cpu caps */
  control->plugin_ctrl.cpucaps = x86cpucaps_getdesc (DEBUG_EXPORT ? 1 : 0);

  /* setup correct streaminfo out of received packet */
  memcpy (&control->streaminfo, addr, sizeof (stream_info_t));
  
#if BUILD_goom
  /* currently we have only goom */
  control->fx_init = fx_goom_init;
  control->fx_step = fx_goom_step;
  control->fx_close = fx_goom_close;
  /* if we're the audio work loop, we init effects plugin */
  if (control->fx_plugin_id != FX_PLUG_ID_NONE)
  {
    current_fx_plugin_id = FX_PLUG_ID_NONE;
    if (control->fx_plugin_id == FX_PLUG_ID_GOOM)
    {
      if (control->
	  fx_init (&control->fx_plugin_ctrl, &control->streaminfo, NULL) == 0)
	/* ok */
	current_fx_plugin_id = control->fx_plugin_id = FX_PLUG_ID_GOOM;
      else
	/* failed */
	control->fx_plugin_id = FX_PLUG_ID_NONE;
    }
  }
#else
  /* no fx plugins avail */
  control->fx_init = NULL;
  control->fx_step = NULL;
  control->fx_close = NULL;
#endif

  /* init playerstatus */
  init_playerstatus (control->stream_type);

#if VSYNC_AUDIO_BENCHMARK
  /* monitor code start */
  hist = rt_mon_hist_create (VSYNC_AUDIO_BENCHMARK_LOW,
      VSYNC_AUDIO_BENCHMARK_HIGH,
      VSYNC_AUDIO_BENCHMARK_BINS,
      "verner/sync/audio", "us", "us", RT_MON_TIMER_SOURCE);
#endif
#if RTMON_DSI_BENCHMARK
  list = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
      RT_MON_EVTYPE_BASIC, 100,
      "verner/sync/audio-dsi", "packet", RT_MON_TIMER_SOURCE, 1);
#endif

#if BUILD_RT_SUPPORT
  LOG ("using Realtime mode for work thread.");

#if !RT_USE_CPU_RESEVE
  /* Get KIP */
  kinfo = l4sigma0_kip_map (L4_INVALID_ID);
  if (!kinfo)
    Panic ("get KIP failed!\n");
#endif

  /* set default period len or user given one */
  if (control->rt_period == 0)
    control->rt_period = RT_DEFAULT_PERIOD;
  ul_usec_period = control->rt_period;

  /* calculate how man frames/chunks per period we have to process */
  packets_per_period =
    streaminfo2loops (&control->streaminfo, ul_usec_period);

  /* noof packets we have still to process in current loop */
  packets_per_period_to_do = packets_per_period;

  /* set default reservation time or user take given */
  if (control->rt_reservation == 0)
    control->rt_reservation = RT_SYNC_AUDIO_EXEC_TIME;
  ul_usec_reservation = control->rt_reservation;

  /* add timeslices */
  /* reservation_time = time for one frame/chunk * packets per period! */
  mand_time = ul_usec_reservation * packets_per_period;	/* microsec per period */
  mand_prio = RT_SYNC_AUDIO_PRIORITY;

  LOGdL (DEBUG_RT_INIT && DEBUG_SYNC, 
      "Adding RT timeslice for mandatory audio part (%i*%lu µsec).",
      packets_per_period, ul_usec_reservation);
  LOGdL (DEBUG_RT_INIT && DEBUG_SYNC, 
      "Setting period length for audio to %lu usecs.",
      ul_usec_period);

#if RT_USE_CPU_RESERVE
  /* add timeslice and set period */
  if (l4cpu_reserve_add (control->work_thread_id, "sync.mand", mand_prio,
	ul_usec_period, &mand_time, ul_usec_period, &mand_id))
    LOG_Error ("Can't add mandatory timeslice.");
  /* start execution */
  if (l4cpu_reserve_begin_minimal_periodic_self (control->work_thread_id))
    LOG_Error ("Could not start periodic mode.");
#else
  /* add timeslice of mandatory part */
  l4_rt_add_timeslice (control->work_thread_id, mand_prio, mand_time);
  /* set period  */
  l4_rt_set_period (control->work_thread_id, ul_usec_period);
  /* start execution */
  l4_rt_begin_minimal_periodic (control->work_thread_id, kinfo->clock + 5000);
#endif

  LOGdL (DEBUG_RT_INIT && DEBUG_SYNC, "processing %d packets per period", 
      packets_per_period_to_do);
#endif // RT support

  /* loop import + export until EOS or Error */
  while (1)
  {
    /* should we stop working */
    if (control->shutdown_work_thread)
      break;

#if VSYNC_AUDIO_BENCHMARK
    /* start point */
    rt_mon_hist_start (hist);
#endif

    /* wait for play */
#if BUILD_RT_SUPPORT
    if ((playmode = check_player_mode (PLAYMODE_PLAY|PLAYMODE_DROP, control->stream_type)) == -1)
    {
      /* just wait the time for one frame/chunk */
      l4thread_usleep (ul_usec_reservation);
      /* goto next iteration */
      continue;
    }
#else
    /* blocks if not playing */
    playmode = check_player_mode (PLAYMODE_PLAY|PLAYMODE_DROP, control->stream_type);
#endif

    if (playmode != PLAYMODE_DROP && !control->out_open)
    {
      /* init export plugin */
      if ((ret = control->out_init (&control->plugin_ctrl, &control->streaminfo)) != 0)
      {
	LOGdL (DEBUG_EXPORT, "ao_oss_init() failed with %d.", ret);
	set_player_mode (PLAYMODE_DROP);
      }
      else
      {
	LOGdL (DEBUG_EXPORT, "Welcome export plugin %s", control->plugin_ctrl.info);
	control->out_open = 1;
	/* set the volumne now */
	if (control->out_setVolume)
	  control->out_setVolume (&control->plugin_ctrl, control->volume_left,
				  control->volume_right);
      }
    }

    if (playmode != PLAYMODE_DROP)
    {
      /*
       * check if should drop some frames or just wait to be in sync,
       * but not if we have still frames to drop
       * only export if we shouldn't drop this frame
       *
       * ATTENTION Don't call metronome_wait_sync here, because it
       * returns immediately if audio with status OK.
       */
      /* check if fx plugin has changed */
      if (control->fx_plugin_id != current_fx_plugin_id)
      {
	/* changed ! */
	/* close old one */
	if (current_fx_plugin_id != FX_PLUG_ID_NONE)
	{
	  if (control->fx_close)
	    control->fx_close (&control->fx_plugin_ctrl);
	}
	
	/* init new one */
	current_fx_plugin_id = FX_PLUG_ID_NONE;
	if (control->fx_plugin_id == FX_PLUG_ID_GOOM)
	{
	  if (control->
	      fx_init (&control->fx_plugin_ctrl, &control->streaminfo,
		NULL) == 0)
	    /* ok */
	    current_fx_plugin_id = control->fx_plugin_id = FX_PLUG_ID_GOOM;
	  else
	    /* failed */
	    control->fx_plugin_id = FX_PLUG_ID_NONE;
	}
      }
      /* use fx plugin */
      if ((current_fx_plugin_id != FX_PLUG_ID_NONE) && (control->fx_step))
	control->fx_step (&control->fx_plugin_ctrl, addr);

      if (control->out_open)
      {
	/* export frame 
	 * for Audio, this does the synchronization. It's basically
	 * a blocking write call to the soundcard, and does only return
	 * when the sound card accepted the sample. Only then we try to
	 * get the next sample from the DSI.
	*/
	if (control->out_step (&control->plugin_ctrl, addr))
	  LOG_Error ("out_step() failed.");
	else
	  LOGdL (DEBUG_EXPORT, "played chunk / display frame ");
	
	/* commit it */
	if (control->out_commit (&control->plugin_ctrl))
	  LOG_Error ("out_commit() failed.");
      }
    }

#if DEBUG_DS
    /* filllevel */
    dsi_filllevel =
      dsi_socket_get_num_committed_packets (control->socket) * 100 /
      dsi_socket_get_packet_num (control->socket);
    if (dsi_filllevel <= 20)
      LOG ("dsi audio-sync: %d%%",
	   dsi_filllevel);
#endif
#if RTMON_DSI_BENCHMARK
    be.time = be.data = dsi_socket_get_num_committed_packets (control->socket);
    rt_mon_list_insert (list, &be);
#endif

    /* immediately acknowledge packet */
    ret = dsi_packet_commit (control->socket, p);
    if (ret)
    {
      Panic ("dsi_packet_commit failed (%d)", ret);
      break;
    }

    /* import next frame */
    /* in RT mode we're using unblocking calls - if no packet is avail sleep a while */
    recv_return = dsi_packet_get (control->socket, &p);
#if BUILD_RT_SUPPORT
    while (recv_return == -DSI_ENOPACKET) 
    {
      /* if we're here, we have no packet avail, we wait the time for one frame/chunk! */
      LOGdL (DEBUG_RT_SLEEP && DEBUG_SYNC, "no packet ready - sleeping for %lu usecs", 
	  ul_usec_reservation);
      /* just wait the time for one frame/chunk */
      l4thread_usleep (ul_usec_reservation);
      /* try to get next packet */
      recv_return = dsi_packet_get (control->socket, &p);
    }
#else
#if VSYNC_NO_AOUT
    /* in non-RT-mode and no-audio, we sleep here for a little, so we do not
     * consume the CPU too much by just consuming packets
     */
    l4_sleep_forever ();
#endif
#endif
    if (recv_return)
    {
      LOG_Error ("dsi_packet_get failed (%x)", recv_return);
      break;
    }

    /* get packet number */
    recv_return = dsi_packet_get_no (control->socket, p, &num);
    if (recv_return)
    {
      Panic ("dsi_packet_get_no failed (%x)", recv_return);
      break;
    }

    /* get packet data */
    recv_return = dsi_packet_get_data (control->socket, p, &addr, &size);
    if (recv_return)
    {
      if (recv_return == -DSI_EEOS)
      {
	LOGdL (DEBUG_IMPORT, "dsi_packet_get_data returned DSI_EEOS");
	break;
      }
      else
      {
	Panic ("dsi_packet_get_data failed (%d)", recv_return);
	break;
      }
    }

#if VSYNC_AUDIO_BENCHMARK
    /* end point */
    rt_mon_hist_end (hist);
#endif

  }				/* end while(1) / work_loop */

#if VSYNC_AUDIO_BENCHMARK
  /* monitor dump histogram */
  //rt_mon_hist_dump (hist);
  /* deregister histogram */
  rt_mon_hist_free (hist);
#endif
#if RTMON_DSI_BENCHMARK
//  rt_mon_list_dump (list);
  rt_mon_list_free (list);
#endif

#if BUILD_RT_SUPPORT
#if RT_USE_CPU_RESERVE
  /* end periodic execution */
  if (l4cpu_reserve_end_periodic (control->work_thread_id))
    LOG_Error("Failed to end periodic mode.");
  /* delete timeslice */
  if (l4cpu_reserve_delete_thread (control->work_thread_id))
    LOG_Error("Failed to delete timeslices.");
#else
  /* end periodic execution */
  l4_rt_end_periodic (control->work_thread_id);
  /* delete timeslice */
  l4_rt_remove (control->work_thread_id);
#endif
#endif

  /* close playerstatus */
  close_playerstatus (control->stream_type);

  /* close export plugin */
  if (control->out_open)
  {
    if (control->out_close (&control->plugin_ctrl))
      LOG_Error ("out_close plugin failed.");
    else
      LOGdL (DEBUG_EXPORT, "export plugin closed.");
    control->out_open = 0;
  }

  /* close effects plugin */
  if ((current_fx_plugin_id != FX_PLUG_ID_NONE) && (control->fx_close))
    control->fx_close (&control->fx_plugin_ctrl);


shutdown_recv_thread:
  /* signal end of stream */
  if (dsi_socket_set_event (control->socket, DSI_EVENT_EOS))
  {
    /* no need to panic as it's normal when not both cores are running */
    LOG_Error ("signal end of stream failed.");
    /* don't try to get last packet */
    recv_return = -DSI_EEOS;
  }

  /* 
   * WE must get last packet for ensuring the sender can deliver it's last packet
   * and it doesn't deadlock !
   */
  /* enable blocking mode for last packets */
#if BUILD_RT_SUPPORT
  control->socket->flags |= DSI_SOCKET_BLOCK;
#endif
  while (recv_return == 0)
  {
    LOGdL (DEBUG_STREAM, "trying to get last packet from sender (ret is %i).",
	   recv_return);
    /* get packet */
    recv_return = dsi_packet_get (control->socket, &p);
    /* check for eos */
    if (!recv_return)
      recv_return = dsi_packet_get_data (control->socket, p, &addr, &size);
    /* commit received */
    if (!recv_return)
      dsi_packet_commit (control->socket, p);
  }

  /* indicate we're down */
  LOGdL (DEBUG_THREAD, "shutdown work_thread done.");
  l4semaphore_up (&control->running_sem);

}

