/**
 * \brief   Work loop for VERNER's video core component
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
#include "receiver.h"
#include "sender.h"
#include "work_loop.h"
#include "types.h"
#include "codecs.h"
#include "postproc.h"
#include "timer.h"
#include "x86cpucaps.h"
#include "qap.h"

/* configuration */
#include "verner_config.h"

#include "work_loop_video.h"

#if BUILD_RT_SUPPORT
/* rt support */
#include <l4/sys/rt_sched.h> // RT scheduling
#include <l4/util/atomic.h>
#include <l4/rmgr/librmgr.h> // for rmgr_set_prio
#include <l4/sigma0/kip.h> // l4sigma0_kip_map
#if RT_USE_CPU_RESERVE
#include <l4/cpu_reserve/sched.h>
#endif

/*
 * optional part (posprocessing)
 * with ugly define magic to use only one option
 */
#if !RT_USE_OPTIONAL
/* disable optional parts */
#define RT_USE_OPTIONAL_FOR_ALL_FRAMES
#define RT_USE_OPTIONAL_FOR_EACH_FRAME
#endif /* use optional */
#endif // RT support

#if VCORE_VIDEO_BENCHMARK || RTMON_DSI_BENCHMARK
/* benchmark */
/* rt_mon stuff */
#include <l4/rt_mon/histogram.h>
#include <l4/rt_mon/event_list.h>

#if BUILD_RT_SUPPORT
extern rt_mon_histogram_t *hist_pipcs;
#endif

/* setup selected timer source for rt_mon */
#if RT_MON_TSC
#define RT_MON_TIMER_SOURCE RT_MON_TSC_TIME
#endif
#if RT_MON_PROCESS
#define RT_MON_TIMER_SOURCE RT_MON_THREAD_TIME
#endif
#endif /* end benchmark */

#if PREDICT_DECODING_TIME_RTMON
#  include <l4/rt_mon/histogram2d.h>
#  include <l4/rt_mon/histogram.h>
#endif

/*****************************************************************************/
/**
 * \brief Work thread for video process.
 * 
 * \param data Thread data
 */
/*****************************************************************************/
void
work_loop_video (void *data)
{
  /* return values from */
  int recv_return = 0;
  int send_return = 0;
  int codec_return = 0;

  /* control structure */
  control_struct_t *control;

  /* noof current send packet */
  int packet_index = 0;

#if VCORE_VIDEO_ENABLE_QAP
  /* loop's until QAP params are changed */
  int loop_cnt = VCORE_VIDEO_QAP_CHECKPOINT;
#endif

#if VCORE_VIDEO_BENCHMARK
  /* this is histogram for whole work-loop */
  rt_mon_histogram_t *hist = NULL;
  rt_mon_histogram_t *hist_decode = NULL;
  rt_mon_histogram_t *hist_filter = NULL;
#endif
#if RTMON_DSI_BENCHMARK
  rt_mon_event_list_t *list_in = NULL;
  rt_mon_event_list_t *list_out = NULL;
  rt_mon_basic_event_t be;
#endif
#if PREDICT_DECODING_TIME_RTMON
  rt_mon_histogram2d_t *hist2d_predict = NULL;
  rt_mon_histogram_t   *hist_overhead  = NULL;
  rt_mon_histogram_t   *hist_abs_error = NULL;
  rt_mon_histogram_t   *hist_rel_error = NULL;
#endif

#if BUILD_RT_SUPPORT
  /* rt support */
  int ret;
  int mand_prio, mand_time;
#if RT_USE_CPU_RESERVE
  int mand_id, opt_id, opt_time;
#else
  l4_kernel_info_t *kinfo = 0;
#endif
  /* the lenght of one period in 탎ec */
  unsigned long ul_usec_period;
  /* the calculated reservation time */
  unsigned long ul_usec_reservation;
  /* the calculated reservation time for postprocessing */
  unsigned long ul_usec_reservation_opt;
  /* how many packets(frames/chunks) we have to process in one period */
  int packets_per_period;
  /* remaining time in a timeslice */
  l4_kernel_clock_t remaining_time;
#endif // RT support

  /*
   * work loop flow control flags 
   */

  /* flag set if codec is initialized */
  int codec_initialized = 0;

  /* this flag is set, if we don't import anymore (EOF), but may be the decoder
     still has data, so decode must be ! */
  short got_import_eos = 0;

  /* this flag is set, if we don't export, but we import, because we need more data
     to en-/decode */
  short disable_export = 0;

  /* this flag is set, if we should not import, because the codec has still enought
     data, so we should only coding_step and may be export */
  short disable_import_has_data = 0;

  /*
   * now begin initialization 
   */

  /* get ptr to control structure */
  control = (control_struct_t *) data;

  /* indicate we're running */
  l4semaphore_down (&control->running_sem);

  /* wait for start event - from sender */
  l4semaphore_down (&control->start_send_sem);
  /* wait for start event - from receiver */
  l4semaphore_down (&control->start_recv_sem);

  LOGdL (DEBUG_THREAD, "work_thread started.");

  /* detecting cpu caps */
  control->plugin_ctrl.cpucaps = x86cpucaps_getdesc (DEBUG_CODEC ? 1 : 0);

  /* get very first incoming packet 
   * (contains stream info, required to set up codec)
   *
   * We have to wait for this packet, therefore:
   * enable blocking mode.
   */
#if BUILD_RT_SUPPORT
  control->recv_socket->flags |= DSI_SOCKET_BLOCK;
#endif
  /* get it */
  recv_return = receiver_step (control);
#if BUILD_RT_SUPPORT
  /* disable blocking mode */
  control->recv_socket->flags &= ~DSI_SOCKET_BLOCK;
#endif
  if (recv_return)
  {
    LOG_Error ("receiving packet failed.");
    goto shutdown_send_thread;
  }

  /* check if we received valid data addr */
  if (control->recv_packet.addr == NULL)
  {
    LOG_Error ("No valid data received. Stop working.");
    goto shutdown_send_thread;
  }

  /* initialize codec if not already done */
  LOGdL (DEBUG_CODEC, "Initializing codec.");
  /* setup correct streaminfo out of received packet */
  memcpy (&control->streaminfo, control->recv_packet.addr,
      sizeof (stream_info_t));
  
  /* now find and setup correct codec */
  if (determineAndSetupCodec (control))
  {
      /* commit already received will be done later */
      LOG_Error ("Initializing codec failed.");
      goto shutdown_send_thread;
  }
  codec_initialized = 1;

#if VCORE_VIDEO_BENCHMARK 
  /* monitor code start */
  hist = rt_mon_hist_create (VCORE_VIDEO_BENCHMARK_LOW,
      VCORE_VIDEO_BENCHMARK_HIGH,
      VCORE_VIDEO_BENCHMARK_BINS,
      "verner/core/video", "us", "us", RT_MON_TIMER_SOURCE);
  hist_decode = rt_mon_hist_create (VCORE_MANDATORY_BENCHMARK_LOW,
      VCORE_MANDATORY_BENCHMARK_HIGH,
      VCORE_MANDATORY_BENCHMARK_BINS,
      "verner/core/video-decode", "us", "us",
      RT_MON_TIMER_SOURCE);
  hist_filter = rt_mon_hist_create (VCORE_OPTIONAL_BENCHMARK_LOW,
      VCORE_OPTIONAL_BENCHMARK_HIGH,
      VCORE_OPTIONAL_BENCHMARK_BINS,
      "verner/core/video-filter", "us", "us",
      RT_MON_TIMER_SOURCE);
#if BUILD_RT_SUPPORT
  hist_pipcs = rt_mon_hist_create (0, 25, 25,
      "verner/core/vpreemption_count", "# preempts", "frequency",
      RT_MON_TIMER_SOURCE);
#endif
#endif
#if RTMON_DSI_BENCHMARK
  list_in = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
      RT_MON_EVTYPE_BASIC, 10,
      "verner/demuxer/video-dsi", "packet", RT_MON_TIMER_SOURCE, 1);
  list_out = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
      RT_MON_EVTYPE_BASIC, 10,
      "verner/sync/video-dsi", "packet", RT_MON_TIMER_SOURCE, 1);
#endif
#if PREDICT_DECODING_TIME_RTMON
  if (control->predict->predictor) {
    double low[2]  = { 0.0, 0.0 };
    double high[2] = { 40000.0, 40000.0 };
    int bins[2]    = { 200, 200 };
    hist2d_predict = rt_mon_hist2d_create(low, high, bins, 1,
      "verner/core/prediction", "us", "us", "count", RT_MON_THREAD_TIME);
    hist_overhead  = rt_mon_hist_create(0.0, 50.0, 100,
      "verner/core/prediction-overhead", "%", "count", RT_MON_THREAD_TIME);
    hist_abs_error = rt_mon_hist_create(-10000.0, 10000.0, 100,
      "verner/core/prediction-absolute-error", "us", "count", RT_MON_THREAD_TIME);
    hist_rel_error = rt_mon_hist_create(0.0, 300.0, 100,
      "verner/core/prediction-relative-error", "%", "count", RT_MON_THREAD_TIME);
  }
#endif

#if BUILD_RT_SUPPORT
  /* set up all RT stuff below */
  LOG ("using Realtime mode for work thread.");

#if !RT_USE_CPU_RESERVE
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

  /* ensure maximum packets in send-buffer is greater or equal then packets_per_period */
  if (MAX_SENDER_PACKETS < packets_per_period)
  {
    Panic("PACKETS_IN_BUFFER<packets_per_period will definitly NOT work."
          " Reconfigure and compile again or decrease period!");
    goto shutdown_send_thread;
  }

  /* set default reservation time or user take given */
  if (control->rt_reservation == 0)
    control->rt_reservation = RT_CORE_VIDEO_EXEC_TIME;
  ul_usec_reservation = control->rt_reservation;
  if (control->rt_reservation_opt == 0)
    control->rt_reservation_opt = RT_CORE_VIDEO_POSTPROC_EXEC_TIME;
  ul_usec_reservation_opt = control->rt_reservation_opt;

  /* add timeslices */
  /* reservation_time = time for one frame/chunk * packets per period! */
  mand_time = ul_usec_reservation * packets_per_period;	/* microsec per period */
  mand_prio = RT_CORE_VIDEO_PRIORITY;

  LOGdL (DEBUG_RT_INIT && DEBUG_VCORE, 
      "Added RT timeslice for mandatory video part (%i*%lu 탎ec).",
      packets_per_period, ul_usec_reservation);
  LOGdL (DEBUG_RT_INIT && DEBUG_VCORE, 
      "Setting period length for video to %lu usecs.",
      ul_usec_period);

#if RT_USE_CPU_RESERVE
  /* add mandatory timeslice and set period */
  if (l4cpu_reserve_add (control->work_thread_id, "vcore.mand", 
	mand_prio, ul_usec_period, &mand_time, ul_usec_period,
	&mand_id))
    LOG_Error ("Can't add mandatory timeslice.");
#else
  /* set period  */
  l4_rt_set_period (control->work_thread_id, ul_usec_period);
  /* add timeslice for mandatory part */
  l4_rt_add_timeslice (control->work_thread_id, mand_prio, mand_time);
#endif

  /* 
   * for video only: we doing postprocessing as optional part 
   */
#if RT_USE_OPTIONAL_FOR_ALL_FRAMES
  /* One optional part for ALL frames per loop */
#if RT_USE_CPU_RESERVE
  opt_time = ul_usec_reservation_opt * packets_per_period;
  if (l4cpu_reserve_add (control->work_thread_id, "vcore.opt",
	RT_CORE_VIDEO_POSTPROC_PRIORITY, ul_usec_period, &opt_time,
	0 /* deadline = 0 denotes optional part */, &opt_id))
    LOG_Error ("Can't add single optional timeslice.");
#else
  l4_rt_add_timeslice (control->work_thread_id, 
      RT_CORE_VIDEO_POSTPROC_PRIORITY,
      ul_usec_reservation_opt * packets_per_period);
#endif
  LOGdL (DEBUG_RT_INIT && DEBUG_VCORE, 
      "Added RT timeslice for ONE optional part (%i 탎ec).",
      ul_usec_reservation_opt * packets_per_period);
#endif
  
#if RT_USE_OPTIONAL_FOR_EACH_FRAME
  /* One optional part for EACH frame per loop */
  for (packet_index = 0; packet_index < packets_per_period; packet_index++)
    {
#if RT_USE_CPU_RESERVE
      /* set opt_time here, because it may be overwritten by cpu_reserve */
      opt_time = ul_usec_reservation_opt;
      if (l4cpu_reserve_add (control->work_thread_id, "vcore.opt",
	    RT_CORE_VIDEO_POSTPROC_PRIORITY - packet_index, ul_usec_period,
	    &opt_time, 0 /* deadline = 0 denotes optional part */, &opt_id))
	LOG_Error ("Can't add optional timeslice %d.", packet_index);
      LOGdL (DEBUG_RT_INIT && DEBUG_VCORE, 
	  "Added RT timeslice for optional part %i (%i 탎ec).",
	  packet_index, opt_time);
#else
      l4_rt_add_timeslice (control->work_thread_id,
	  RT_CORE_VIDEO_POSTPROC_PRIORITY - packet_index,
	  ul_usec_reservation_opt);
      LOGdL (DEBUG_RT_INIT && DEBUG_VCORE, 
	  "Added RT timeslice for optional part %i (%i 탎ec).",
	  packet_index, ul_usec_reservation_opt);
#endif
    }
#endif
  
#if RT_USE_CPU_RESERVE
  /* start execution */
  if (l4cpu_reserve_begin_minimal_periodic_self (control->work_thread_id))
    LOG_Error ("Could not start periodic mode");
#else
  /* start execution */
  l4_rt_begin_minimal_periodic (control->work_thread_id, kinfo->clock + 5000);
#endif
#endif // RT support

  /* 
   * work loop - description:
   *
   * while(1)
   * {
   *   recalculate QAP params
   *  
   *   mandatory part (DECODE): 
   *   mA) next_period (RT only)
   *   for(all packets_per_period) (RT only)
   *   {
   *     mB) recv_packet_get
   *     mC) send_packet_get
   *     mD) decode
   *     mE) recv_commit
   *   }
   *
   *   optional part (POSTPROC):
   *   oA1) next_reservation (RT only) (*)
   *   for(all packets_per_period) (RT only)
   *   {
   *     oA2) next_reservation (RT only) (*)
   *     oB) postproc
   *     oC) send_commit
   *   }
   *
   * } end while(1) 
   *
   * (*) depends on compile mode
   *     Either whole postproc is one optional part or
   *     postproc for each frame is one
   *
   * now we really begin it :)
   */

  while (1)
  {
#if BUILD_RT_SUPPORT
begin_while_loop:
#endif

    /* should we stop working */
    if (control->shutdown_work_thread)
      break;

#if VCORE_VIDEO_BENCHMARK
    /* start point */
    rt_mon_hist_start (hist);
#endif

/* 
 * mandatory part follow
 */


#if VCORE_VIDEO_ENABLE_QAP
/* 
 * --------- recalculate QAP params ---------- 
 */
    /* QAP - only for video */
#if BUILD_RT_SUPPORT
    loop_cnt -= packets_per_period;
#else
    loop_cnt--;
#endif
    if (loop_cnt <= 0)
    {
      /* check QAP settings */
      if (control->qap_settings.useQAP)
	/* recalculate quality level */
	QAP (control);
      /* recheck after QAP_CHECKPOINT frames */
      loop_cnt = VCORE_VIDEO_QAP_CHECKPOINT;
    }
#endif /* video-enable QAP */

#if BUILD_RT_SUPPORT
    /*
     * next_period (starting here w/ mandatory part)
     */
    LOGdL (DEBUG_RT_NEXT_PERIOD && DEBUG_VCORE, 
	"Calling next_period...");
    if ((ret = l4_rt_next_period()) != 0)
      LOG_Error ("next_period returned %d", ret);

    /* for all packet's in this period */
    for (packet_index = 0; packet_index < packets_per_period; packet_index++)
    {
#endif // RT support
      /* check for EOS ( recv_return set if import failed! ) */
      if ((recv_return) && (recv_return != -DSI_ENOPACKET))
      {
	LOGdL (DEBUG_STREAM, "nothing to import anymore. Got EOS.");
	/* nothing to import, but may be to decode! */
	got_import_eos = 1;
      }

      /* import next frame, if not EOS or codec has still data to code */
      if ((!got_import_eos) && (!disable_import_has_data))
      {
	/* get incoming packet */
	recv_return = receiver_step (control);
#if BUILD_RT_DUPPORT
	if (recv_return == -DSI_ENOPACKET)
	{
	  if (packet_index < packets_per_period -1)
	  {
	    LOGdL (DEBUG_RT_SLEEP && DEBUG_VCORE, "Sleeping for %lu usecs", 
		ul_usec_reservation);
	    l4thread_usleep (ul_usec_reservation);
	  }
	  else
	    goto begin_while_loop;
	  /* either we increment the packet count and get the next
	   * packet or we exit the for loop and potentially call
	   * next_period.
	   */
	  continue; // basically jumps to begin of for loop
	}
#endif
	
	/* we reach this point only if either we got a packet,
	 * or we got an error != -DSI_ENOPACKET
	 */
	if (recv_return)
	{
	  LOGdL (DEBUG_CODEC,
		 "got no frame to decode - expect EOS. Disable import.");
	  got_import_eos = 1;
	  control->recv_packet.addr = NULL;
	}
      }
      /* avoid decoding last received packet forever :) */
      else
	control->recv_packet.addr = NULL;

      /* should we export - then get outgoing packet */
      if (!disable_export)
      {
	send_return = sender_step (control, packet_index);
#if BUILD_RT_SUPPORT
	while (send_return == -DSI_ENOPACKET)
	{
	  /* if we're here, we have no packet avail, we wait the time for one frame/chunk! 
	   * if this is the last packet for this period, call next_period,
      	   * otherwise wait the amount of time of one packet
	   */
	  if (packet_index < packets_per_period - 1)
	  {
	    LOGdL (DEBUG_RT_SLEEP && DEBUG_VCORE, "Sleeping for %lu usecs", 
		ul_usec_reservation);
	    l4thread_usleep (ul_usec_reservation);
	  }
	  else
	    goto begin_while_loop;
	  /* try again to get one packet */
	  send_return = sender_step (control, packet_index);
	}
#endif
	
	/* we reach this point only if either we got a packet,
	 * then send_return is 0, or we got an error != -DSI_ENOPACKET
	 */
	if (send_return)
	{
	  /* failure - no chance to work on */
	  LOGdL (DEBUG_CODEC, "got no frame to encode - expect EOS.");
	  goto end_while_loop; // break out of while loop
	}
      }

#if PREDICT_DECODING_TIME
      {
	l4_int64_t prediction = 0;
	if (control->predict->predictor) {
	  unsigned char *data = NULL;
	  unsigned length     = 0;
	  if (!disable_import_has_data && control->recv_packet.addr) {
	    data = control->recv_packet.addr + sizeof(frame_ctrl_t);
	    length = ((frame_ctrl_t *)control->recv_packet.addr)->framesize;
	  }
	  control->predict->prediction_overhead -= get_thread_time_microsec();
	  prediction = control->predict->predict(control->predict->predictor, &data, &length) * 1E6;
	  control->predict->prediction_overhead += get_thread_time_microsec();
	  control->predict->decoding_time -= get_thread_time_microsec();
	}
#endif
      /* decode or encode frame */
#if VCORE_VIDEO_BENCHMARK
      /* start point */
      rt_mon_hist_start (hist_decode);
#endif
      codec_return =
	control->codec_step (&control->plugin_ctrl,
			     disable_import_has_data ? NULL : control->recv_packet.addr,
			     control->send_packet[packet_index].addr);
#if VCORE_VIDEO_BENCHMARK
      /* end point */
      rt_mon_hist_end (hist_decode);
#endif
#if PREDICT_DECODING_TIME
	if (control->predict->predictor) {
	  control->predict->decoding_time += get_thread_time_microsec();
	  if (prediction != -1000000) {
#if PREDICT_DECODING_TIME_RTMON
	    rt_mon_time_t t[2];
	    t[0] = control->predict->decoding_time;
	    t[1] = prediction;
	    rt_mon_hist2d_insert_data(hist2d_predict, t, 0, 1);
	    rt_mon_hist_insert_data(hist_overhead,
	      (double)control->predict->prediction_overhead / (double)control->predict->decoding_time * 1E2, 1);
	    rt_mon_hist_insert_data(hist_abs_error,
	      prediction - control->predict->decoding_time, 1);
	    rt_mon_hist_insert_data(hist_rel_error,
	      (double)prediction / (double)control->predict->decoding_time * 1E2, 1);
#endif
	    printf("(prediction, actual, overhead) in us: %lld, %lld, %lld\n",
	      prediction, control->predict->decoding_time, control->predict->prediction_overhead);
	  }
	  if (!is_CODING_MORE(codec_return)) {
	    control->predict->predict_learn(control->predict->predictor, (double)control->predict->decoding_time / 1E6);
	    control->predict->decoding_time       = 0;
	    control->predict->prediction_overhead = 0;
	  }
	}
      }
#endif

      /* check return values */
      if (is_CODING_MORE(codec_return))
      {
	if (got_import_eos)
	{
	  /* you won't get more data, if we have nothing to import - exit */
	  disable_export = 1;
	  LOGdL (DEBUG_CODEC, "we need more data, but import is disabled :(\n"
                              "Can't continue.");
	  goto end_while_loop;
	}
	else
	{
	  /* we need more data */
	  disable_export = 1;
	  disable_import_has_data = 0;
	  LOGdL (DEBUG_CODEC, "we need more data.");
	}
      }
      else if (is_CODING_OK(codec_return))
      {
	/* set packet state to coded (means it contains valid data - so we have to commit this one */
	control->send_packet[packet_index].status = STATE_PACKET_FILLED;

	if (has_CODING_DATA(codec_return))
	{
	  /* 
	   * code says, he has still something to de/encode, 
	   * so we don't need to import a new packet!
	   */
	  LOGdL (DEBUG_CODEC, "Codec has still data.");
	  disable_import_has_data = 1;
	}

	/* coding was sucessfull */
	LOGdL (DEBUG_CODEC, "Coding frame was ok. :)");
	/* export this frame */
	disable_export = 0;
      }				/* end CODING_OK */
      else
      {
	/* failure in codec step! */
	LOGdL (DEBUG_CODEC, "Coding frame failed! Aborting (%d).",
	       codec_return);
	goto end_while_loop;
      }

      /* commit incoming packet */
      if ((!got_import_eos) && (!disable_import_has_data))
      {
	if (receiver_commit (control))
	{
	  LOG_Error ("receiver_commit failed.");
	  goto end_while_loop;
	}

#if RTMON_DSI_BENCHMARK
	/* when we committed iincoming packet, benchmark this event */
	be.time = be.data = dsi_socket_get_num_committed_packets (control->recv_socket);
	rt_mon_list_insert (list_in, &be);
#endif
      } /* commit incoming packet */
#if BUILD_RT_SUPPORT
    } /* end for all packet_per_period mandatory */
#endif

/* 
 * optional part below
 */

#if BUILD_RT_SUPPORT
#if RT_USE_OPTIONAL_FOR_ALL_FRAMES
    /* in rt-mode the postprocessing is the optional part (video only) */
    LOGdL (DEBUG_RT_NEXT_RESERV && DEBUG_VCORE, 
	"Calling next_reservation for all packets");
    l4_rt_next_reservation (1, &remaining_time);
#endif

    /* for all packet's in this period */
    for (packet_index = 0; packet_index < packets_per_period; packet_index++)
    {
#if RT_USE_OPTIONAL_FOR_EACH_FRAME
	/* in rt-mode the postprocessing is the optional part (video only) */
	LOGdL (DEBUG_RT_NEXT_RESERV && DEBUG_VCORE, 
	    "Calling next_reservationi for packet %d", packet_index)
	l4_rt_next_reservation (1 + packet_index, &remaining_time);
#endif
#endif // RT support

        /* doing postprocessing */
#if VCORE_VIDEO_BENCHMARK
        /* start point */
	rt_mon_hist_start (hist_filter);
#endif
	/* checks for video type inside */
        postProcessEngineStep (control,
                               control->send_packet[packet_index].addr);
#if VCORE_VIDEO_BENCHMARK
	/* end point */
        rt_mon_hist_end (hist_filter);
#endif

        /* commit outgoing packet */
        if (!disable_export)
        {
          if (sender_commit (control, packet_index))
          {
            LOG_Error ("sender_commit failed.");
	    goto end_while_loop;
          }
	  
#if RTMON_DSI_BENCHMARK
	/* when we got rid of an out packet, benchmark it */
	be.time = be.data = dsi_socket_get_num_committed_packets (control->send_socket);
	rt_mon_list_insert (list_out, &be);
#endif
        } // !disable_export
#if BUILD_RT_SUPPORT
    }	/* end for all packet_per_period optional */
#endif

#if VCORE_VIDEO_BENCHMARK
    /* end point */
    rt_mon_hist_end (hist);
#endif

  }				/* end while(1) / work_loop */
end_while_loop: 

  /*
   * work is done:
   * now we have to close all and release everything
   */
#if BUILD_RT_SUPPORT
#if RT_USE_CPU_RESERVE
  /* end periodic execution */
  if (l4cpu_reserve_end_periodic (control->work_thread_id))
    LOG_Error("Failed to end periodic mode.");
  /* delete timeslices */
  if (l4cpu_reserve_delete_thread (control->work_thread_id))
    LOG_Error("Failed to delete timeslices.");
#else
  /* end periodic execution */
  l4_rt_end_periodic (control->work_thread_id);
  /* delete timeslices */
  l4_rt_remove (control->work_thread_id);
#endif
#endif

shutdown_send_thread:

#if VCORE_VIDEO_BENCHMARK
  /* monitor dump histogram */
  //rt_mon_hist_dump (hist);
  /* deregister histogram */
  if (hist)
    rt_mon_hist_free (hist);
  /* monitor dump histogram */
  //rt_mon_hist_dump (hist_filter);
  if (hist_decode)
    rt_mon_hist_free (hist_decode);
  if (hist_filter)
    rt_mon_hist_free (hist_filter);
#if BUILD_RT_SUPPORT
  rt_mon_hist_free (hist_pipcs);
#endif
#endif
#if RTMON_DSI_BENCHMARK
  if (list_in)
    rt_mon_list_free (list_in);
  if (list_out)
    rt_mon_list_free (list_out);
#endif
#if PREDICT_DECODING_TIME_RTMON
  if (control->predict->predictor) {
    rt_mon_hist2d_free (hist2d_predict);
    rt_mon_hist_free (hist_overhead);
    rt_mon_hist_free (hist_abs_error);
    rt_mon_hist_free (hist_rel_error);
  }
#endif
  
  /* close codec if initialized */
  if (codec_initialized)
  {
    if (control->codec_close (&control->plugin_ctrl))
      LOG_Error ("Closing import plugin failed.");
    else
      LOGdL (DEBUG_CODEC, "Codec plugin closed.");
  }

#if PREDICT_DECODING_TIME
  /* evaluate the predictor */
  if (control->predict->predictor)
    control->predict->predict_eval(control->predict->predictor);
#endif

  /* always call commit to ensure we commit the last packet */
  receiver_commit (control);

  /* signal eos to ctrl via dsi_send_event */
  receiver_signal_eos (control);

  /* 
   * signal end of stream to receiving client 
   * also it commits all pending valid packets 
   */
  sender_signal_eos (control);

  /* close filter */
  postProcessEngineClose ();

  /*
   * WE must get last packet for ensuring the sender can deliver it's last packet
   * and it doesn't deadlock !
   */
  while (recv_return == 0)
  {
    LOGdL (DEBUG_STREAM, "trying to get last packet from sender (ret is %i).",
	   recv_return);
    recv_return = receiver_step (control);
    if (!recv_return)
    {
      if (receiver_commit (control))
	LOG_Error ("commit failed.");
    }
  }
  /* declare work thread as non exitent for work_loop_create() and work_loop_stop */
  control->work_thread = -1;

  /* indicate we're down */
  LOGdL (DEBUG_THREAD, "shutdown work_thread done.");
  l4semaphore_up (&control->running_sem);
}

