/*
 * \brief   Sender for VERNER's demuxer component
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

/* verner */
#include "arch_globals.h"

/* local */
#include "timer.h"
#include "work_loop.h"
#include "container.h"

/* configuration */
#include "verner_config.h"

#if BUILD_RT_SUPPORT
/* rt support */
#include <l4/sys/rt_sched.h> // RT scheduling
#include <l4/rmgr/librmgr.h> // rmgr_set_prio
#include <l4/sigma0/kip.h> // l4sigma0_kip_map
#if RT_USE_CPU_RESERVE
#include <l4/cpu_reserve/sched.h>
#endif
#endif

#if VDEMUXER_VIDEO_BENCHMARK || VDEMUXER_AUDIO_BENCHMARK || RTMON_DSI_BENCHMARK
/* benchmark */
/* rt_mon stuff */
#include <l4/rt_mon/event_list.h>
#include <l4/rt_mon/histogram.h>
/* setup selected timer source for rt_mon */
#if RT_MON_TSC
#define RT_MON_TIMER_SOURCE RT_MON_TSC_TIME
#endif
#if RT_MON_PROCESS
#define RT_MON_TIMER_SOURCE RT_MON_THREAD_TIME
#endif
#endif /* end benchmark */

#if VDEMUXER_PETZE
#include <l4/petze/petzetrigger.h>
#endif

/*****************************************************************************/
/**
 * \brief	dsi helper - send EOS on DSI stream.
 *
 * \param	socket	the socket the EOS should be sent on
 * \parame	packet  the packet that should be used (if packet_got is 1)
 * \param	offset of packet
 * \param	packet_got if dsi_packet_get is already called then set to 1
 * \return	0 on success
 */
/*****************************************************************************/
static int
send_eos (dsi_socket_t * socket, dsi_packet_t * packet, unsigned long off,
	  int packet_got)
{
  int ret;

  LOGdL (DEBUG_STREAM, "Stopping stream, sending EOS...");
  if (!packet_got)
  {
#if BUILD_RT_SUPPORT
    /* enable blocking mode for last packet */
    socket->flags |= DSI_SOCKET_BLOCK;
#endif
    ret = dsi_packet_get (socket, &packet);
    if (ret)
    {
      LOG_Error ("dsi_packet_get() returned %d", ret);
      return ret;
    }
  }
  ret = dsi_packet_set_no (socket, packet, off);
  if (ret)
  {
    LOG_Error ("dsi_packet_get() returned %d", ret);
    return ret;
  }
  dsi_packet_add_data (socket, packet, (l4_addr_t) NULL, 0,
		       DSI_DATA_AREA_EOS);
  if (ret)
  {
    LOG_Error ("dsi_packet_add_data() returned %d", ret);
    return ret;
  }

  /* commit packet */
  ret = dsi_packet_commit (socket, packet);
  if (ret)
  {
    LOG_Error ("dsi_packet_commit() returned %d\n", ret);
    return ret;
  }

  LOGdL (DEBUG_STREAM, "Stopping stream, sending EOS done.");
  return 0;
}

#if BUILD_RT_SUPPORT
/*****************************************************************************/
/**
 * \brief Preempter thread.
 * 
 * \param data Thread data
 */
/*****************************************************************************/
static void
preempter_thread (void *data)
{
  l4_umword_t word1, word2;
  l4_msgdope_t result;
  int exp, man;

  control_struct_t *control;
  /* get ptr to control structure */
  control = (control_struct_t *) data;

  /* indicate we're running */
  l4semaphore_down (&control->running_preempt);

  /* calculate timeout for ipc_receive  to kill this thread */
  l4util_micros2l4to (1000000, &man, &exp);

  while (1)
  {
    /* should we stop working */
    if (control->shutdown_work_thread)
      break;

    /* wait for preemption IPC */
    if (l4_ipc_receive (l4_preemption_id (control->work_thread_id),
			L4_IPC_SHORT_MSG, &word1, &word2,
			/* L4_IPC_NEVER, <-- don't use as we stop preempter thread */
			L4_IPC_TIMEOUT (0, 0, man, exp, 0, 0), &result) == 0)
    {
      l4util_inc32 (&control->rt_preempt_count);
#if !(DEBUG_RT_PREEMPT && DEBUG_DEMUXER)
      if (control->rt_verbose_pipc != 0)
      {
#endif
	LOG ("%s: received P-IPC (Type:%i (%s) ID:%u, Time:%llu)",
	     control->stream_type == STREAM_TYPE_VIDEO ? "(V)" : "(A)",
	     (word2 & 0x80000000U) >> 31,
	     ((word2 & 0x80000000U) >> 31) == 1 ? "TS overrun" : "DL miss",
	     (word2 & 0x7f000000U) >> 24,
	     ((unsigned long long) word2 << 32 | word1) &
	     0xffffffffffffffULL);
#if !(DEBUG_RT_PREEMPT && DEBUG_DEMUXER)
      }
#endif
    }
  }
  /* indicate we're down */
  l4semaphore_up (&control->running_preempt);

  LOGdL (DEBUG_RT_PREEMPT && DEBUG_DEMUXER, "preemption thread killed");
}
#endif

/*****************************************************************************/
/**
 * \brief Work thread.
 * 
 * \param data Thread data (unused)
 */
/*****************************************************************************/
static void
work_loop (void *data)
{
  int ret;
  dsi_packet_t *p;
  void *start_addr, *dst_addr;
  l4_size_t dst_size;
  int count = 0;
  /* packetsize per packet */
  int packetsize = 0;
  /* 
   * indication if last packet is sucessfully commited (for send_eos) to avoid calling 
   * dsi_packet_get twice w/ only one commit
   */
  int is_last_packet_commited = 1;

  control_struct_t *control;

#if VDEMUXER_VIDEO_BENCHMARK || VDEMUXER_AUDIO_BENCHMARK
  rt_mon_histogram_t *hist = 0;
#if BUILD_RT_SUPPORT
  rt_mon_histogram_t *hist_pipcs;
#endif
#endif

#if RTMON_DSI_BENCHMARK
  rt_mon_event_list_t *list = 0;
  rt_mon_basic_event_t be;
#endif

#if BUILD_RT_SUPPORT
  /* rt support */
  int mand_prio;
  int mand_time;
#if RT_USE_CPU_RESERVE
  int mand_id;
#endif
  l4_kernel_info_t *kinfo = 0;
  /* the lenght of one period in µsec */
  unsigned long ul_usec_period;
  /* the calculated reservation time */
  unsigned long ul_usec_reservation;
  /* how many packets(frames/chunks) we have to process in one period */
  int packets_per_period;
  /* noof packets we have still to process in current loop */
  int packets_per_period_to_do;
#endif

#if VDEMUXER_PETZE
  unsigned long petze_count = 0;
#endif

  /* get ptr to control structure */
  control = (control_struct_t *) data;

  /* indicate we're running */
  l4semaphore_down (&control->running_sem);

  /* wait for start */
  l4semaphore_down (&control->start_sem);

  /* get data area */
  ret = dsi_socket_get_data_area (control->socket, &start_addr, &dst_size);
  if (ret)
  {
    LOG_Error ("work_loop(): get data area failed (%d)", ret);
    goto shutdown_work_loop;
  }

  /* touch */
  l4_touch_rw (start_addr, dst_size);

  /* set start addr of mapped dataspace */
  dst_addr = start_addr;

  /* calculate reserved packetsize per packet ! */
  packetsize = (dst_size / dsi_socket_get_packet_num (control->socket));

  /* check if we should go down before using any socket ! */
  if (control->shutdown_work_thread)
    goto shutdown_work_loop;

  /* init import plugin */
  if (determineAndSetupImport (control))
  {
    LOG_Error ("determineAndSetup() failed.");
    goto shutdown_work_loop;
  }
  else
    LOGdL (DEBUG_IMPORT, "Welcome import plugin %s.",
	   control->plugin_ctrl.info);

  /* started */
  LOGdL (DEBUG_THREAD, "work thread started.");

#if VDEMUXER_VIDEO_BENCHMARK
  /* monitor code start */
  if (control->stream_type == STREAM_TYPE_VIDEO && VDEMUXER_VIDEO_BENCHMARK)
  {
    /* video work_loop */
    hist = rt_mon_hist_create (VDEMUXER_VIDEO_BENCHMARK_LOW,
			       VDEMUXER_VIDEO_BENCHMARK_HIGH,
			       VDEMUXER_VIDEO_BENCHMARK_BINS,
			       "verner/demuxer/video", "time [us]", "occ.",
			       RT_MON_TIMER_SOURCE);
#if BUILD_RT_SUPPORT
    hist_pipcs = rt_mon_hist_create (0, 25, 25,
	"verner/demuxer/vpreemption_count", "# preempts", "frequency",
	RT_MON_TIMER_SOURCE);
#endif /* BUILD_RT_SUPPORT */
  }
#endif /* VDEMUXER_VIDEO_BENCHMARK */
#if VDEMUXER_AUDIO_BENCHMARK
  if (control->stream_type == STREAM_TYPE_AUDIO && VDEMUXER_AUDIO_BENCHMARK)
  {
    /* audio work_loop */
    hist = rt_mon_hist_create (VDEMUXER_AUDIO_BENCHMARK_LOW,
			       VDEMUXER_AUDIO_BENCHMARK_HIGH,
			       VDEMUXER_AUDIO_BENCHMARK_BINS,
			       "verner/demuxer/audio", "time [us]", "occ.",
			       RT_MON_TIMER_SOURCE);
#if BUILD_RT_SUPPORT
    hist_pipcs = rt_mon_hist_create (0, 25, 25,
	"verner/demuxer/apreemption_count", "# preempts", "frequency",
	RT_MON_TIMER_SOURCE);
#endif /* BUILD_RT_SUPPORT */
  }
#endif /* VDEMUXER_AUDIO_BENCHMARK */

#if RTMON_DSI_BENCHMARK
  if (control->stream_type == STREAM_TYPE_VIDEO)
    {
      list = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
	  RT_MON_EVTYPE_BASIC, 100,
	  "verner/demuxer/video-dsi", "packet", RT_MON_TIMER_SOURCE, 1);
    }
  else
    {
      list = rt_mon_list_create(sizeof(rt_mon_basic_event_t),
	  RT_MON_EVTYPE_BASIC, 100,
	  "verner/demuxer/audio-dsi", "packet", RT_MON_TIMER_SOURCE, 1);
    }
#endif

#if BUILD_RT_SUPPORT
  LOG ("using Realtime mode for work thread.");

  /* Setting priority for worker */
//  rmgr_set_prio (control->work_thread_id,
//	    control->stream_type ==
//	    STREAM_TYPE_VIDEO ? RT_DEMUXER_VIDEO_PRIORITY :
//	    RT_DEMUXER_AUDIO_PRIORITY);

  /* Get KIP */
  kinfo = l4sigma0_kip_map (L4_INVALID_ID);
  if (!kinfo)
    Panic ("get KIP failed!\n");

  /* set default period len or user given one */
  if (control->rt_period != 0)
    ul_usec_period = control->rt_period;
  else
    ul_usec_period = RT_DEFAULT_PERIOD;

  /* calculate how man frames/chunks per period we have to process */
  packets_per_period =
    streaminfo2loops (&control->streaminfo, ul_usec_period);

  /* noof packets we have still to process in current loop */
  packets_per_period_to_do = packets_per_period;

  /* set default reservation time or user take given */
  if (control->rt_reservation != 0)
    ul_usec_reservation = control->rt_reservation;
  else
    ul_usec_reservation =
      (control->stream_type ==
       STREAM_TYPE_VIDEO ? RT_DEMUXER_VIDEO_EXEC_TIME :
       RT_DEMUXER_AUDIO_EXEC_TIME);


  /* add timeslices */
  /* reservation_time = time for one frame/chunk * packets per period! */
  mand_prio =
    control->stream_type ==
    STREAM_TYPE_VIDEO ? RT_DEMUXER_VIDEO_PRIORITY : RT_DEMUXER_AUDIO_PRIORITY;
  mand_time = ul_usec_reservation * packets_per_period;
  
  LOGdL (DEBUG_RT_INIT && DEBUG_DEMUXER, 
      "Adding RT timeslice for mandatory %s part (%i*%lu µsec).",
      control->stream_type == STREAM_TYPE_VIDEO ? "video" : "audio",
      packets_per_period, ul_usec_reservation);
  LOGdL (DEBUG_RT_INIT && DEBUG_DEMUXER, 
      "Setting period length for %s to %lu usecs.",
      control->stream_type == STREAM_TYPE_VIDEO ? "video" : "audio",
      ul_usec_period);
  
#if RT_USE_CPU_RESERVE
  /* add mandatory timeslice and set period */
  if (l4cpu_reserve_add (control->work_thread_id, "demux.mand", mand_prio,
	ul_usec_period, &mand_time, ul_usec_period, &mand_id))
    LOG_Error ("Can't add mandatory timeslice.");
  /* start execution */
  if (l4cpu_reserve_begin_minimal_periodic_self (control->work_thread_id))
      LOG_Error ("Could not start periodic mode.");
#else
  /* add mandatory timeslice */
  l4_rt_add_timeslice (control->work_thread_id, mand_prio, mand_time);
  /* set period */
  l4_rt_set_period (control->work_thread_id, ul_usec_period);
  /* start execution */
  l4_rt_begin_minimal_periodic (control->work_thread_id, kinfo->clock + 500000);
#endif
#endif /* end RT_SUPPORT */

  /*
   * the work loop itself
   */
  while (1)			/* until EOS or error */
  {

#if DEBUG_DS
    /* dsi_fill-level */
    int dsi_filllevel;
#endif

#if VDEMUXER_PETZE
    if ((++petze_count % 25) == 0)
    {
      petze_dump();
      petze_count = 0;
    }
#endif

    /* should we stop working */
    if (control->shutdown_work_thread)
      {
	LOGdL (DEBUG_THREAD, "Exiting loop, because shutting down.");
	break;
      }

#if VDEMUXER_VIDEO_BENCHMARK || VDEMUXER_AUDIO_BENCHMARK
    /* start point */
    if (control->stream_type == STREAM_TYPE_VIDEO && VDEMUXER_VIDEO_BENCHMARK)
	rt_mon_hist_start (hist);
    if (control->stream_type == STREAM_TYPE_AUDIO && VDEMUXER_AUDIO_BENCHMARK)
	rt_mon_hist_start (hist);
#endif


#if BUILD_RT_SUPPORT
    /* we finished another packet 
     * (whether we decrement here or at end of loop is only
     * interesting for the very first iteration)
     */
    packets_per_period_to_do--;
    /* next_period */
    if (packets_per_period_to_do <= 0)
    {
      /* reset preemption counter */
#if VDEMUXER_VIDEO_BENCHMARK || VDEMUXER_AUDIO_BENCHMARK
      if (control->stream_type == STREAM_TYPE_VIDEO && VDEMUXER_VIDEO_BENCHMARK)
        rt_mon_hist_insert_data(hist_pipcs, control->rt_preempt_count, 1);
      if (control->stream_type == STREAM_TYPE_AUDIO && VDEMUXER_AUDIO_BENCHMARK)
        rt_mon_hist_insert_data(hist_pipcs, control->rt_preempt_count, 1);
#endif
      control->rt_preempt_count = 0;
      
      LOGdL (DEBUG_RT_NEXT_PERIOD && DEBUG_DEMUXER, "next_period call");
      if ((ret = l4_rt_next_period()) != 0)
	LOG_Error ("next_period returned %x", ret);
      /* noof packets we have still to process in current loop */
      packets_per_period_to_do = packets_per_period;
    }
#endif

    /* get packet descriptor */
    ret = dsi_packet_get (control->socket, &p);
#if BUILD_RT_SUPPORT
    /* in RT mode we're using unblocking calls 
     * if no packet is available: 
     * - sleep a while if there is time left (more than 1 packet to go)
     *   (otherwise we get timeslice overrun)
     * - call next_period otherwise
     */
    while ((ret == -DSI_ENOPACKET) &&
	   (packets_per_period_to_do > 1))
    {
      /* if we're here, we have no packet avail, we wait the time for one frame/chunk! */
      LOGdL (DEBUG_RT_SLEEP && DEBUG_DEMUXER, "no packet ready - sleeping for %lu usecs", 
	  ul_usec_reservation);
      packets_per_period_to_do--;
      /* just wait the time for one frame/chunk */
      l4thread_usleep (ul_usec_reservation);
      /* try again to get one packet */
      ret = dsi_packet_get (control->socket, &p);
    }
    /* if ret is still -DSI_ENOPACKET here, then we
     * should call next_period
     */
    if (ret == -DSI_ENOPACKET)
      continue;		/* call next_period! */
#endif
    if (ret)
    {
      LOG_Error ("get packet failed (%d)", ret);
      break;
    }
    is_last_packet_commited = 0;

    /* we don't seek - so let's import one frame */
    if (!control->seek)
    {
      /* import step() - demuxing frame into dst_addr-Packet */
      ret = control->import_step (&control->plugin_ctrl, dst_addr);
      /* if we get an no packet available, then we restart loop
       * and try to get next packet
       */
    }
    else
      /* we got an seek command */
    {
      ret =
	control->import_seek (&control->plugin_ctrl, dst_addr,
			      control->seek_position, control->seek_whence);
      /* command executed */
      control->seek = 0;
    }
    /* check imports return value */
    if (ret)
    {
      LOGdL (DEBUG_IMPORT, "import frame failed (%d) - EOS ?", ret);
      break;
    }

    /* add data */
    ret =
      dsi_packet_add_data (control->socket, p, dst_addr, PACKETS_IN_BUFFER,
			   0);
    if (ret)
    {
      LOG_Error ("add data failed (%d)", ret);
      break;
    }

    /* set packet number */
    ret = dsi_packet_set_no (control->socket, p, count++);
    if (ret)
    {
      LOG_Error ("set packet number failed (%d)", ret);
      break;
    }

    /* commit import */
    control->import_commit (&control->plugin_ctrl);

    /* commit packet */
    ret = dsi_packet_commit (control->socket, p);
    if (ret)
    {
      LOG_Error ("commit packet failed (%d)", ret);
      break;
    }
    is_last_packet_commited = 1;

    /* 
     * check for wrap-around. If the data area size
     * corresponds to the maximum number of packets and their
     * size, we should not overwrite our own data here 
     */
    dst_addr += control->plugin_ctrl.packetsize;
    if ((dst_addr + packetsize) > (start_addr + dst_size))
      dst_addr = start_addr;

#if DEBUG_DS
    /* enable if you have problems with invalid mem access! */
    if (control->plugin_ctrl.packetsize > packetsize)
      Panic ("Current packet > default packet!!");

    /* filllevel */
    dsi_filllevel =
      dsi_socket_get_num_committed_packets (control->socket) * 100 /
      dsi_socket_get_packet_num (control->socket);
    if (dsi_filllevel <= 20)
      LOG ("dsi demux: %d%%", dsi_filllevel);
#endif

#if RTMON_DSI_BENCHMARK
    be.time = be.data = dsi_socket_get_num_committed_packets (control->socket);
    rt_mon_list_insert (list, &be);
#endif

#if VDEMUXER_VIDEO_BENCHMARK || VDEMUXER_AUDIO_BENCHMARK
    /* end point */
    if (control->stream_type == STREAM_TYPE_VIDEO && VDEMUXER_VIDEO_BENCHMARK)
      rt_mon_hist_end (hist);
    if (control->stream_type == STREAM_TYPE_AUDIO && VDEMUXER_AUDIO_BENCHMARK)
      rt_mon_hist_end (hist);
#endif

  }				/* end while(1) / work_loop */

#if VDEMUXER_PETZE
  petze_dump();
#endif

#if VDEMUXER_VIDEO_BENCHMARK || VDEMUXER_AUDIO_BENCHMARK
  /* monitor dump histogram */
  //rt_mon_hist_dump (hist);
  /* deregister histogram */
  if (control->stream_type == STREAM_TYPE_VIDEO && VDEMUXER_VIDEO_BENCHMARK)
    rt_mon_hist_free (hist);
  if (control->stream_type == STREAM_TYPE_AUDIO && VDEMUXER_AUDIO_BENCHMARK)
    rt_mon_hist_free (hist);
#if BUILD_RT_SUPPORT
  if (control->stream_type == STREAM_TYPE_VIDEO && VDEMUXER_VIDEO_BENCHMARK)
    rt_mon_hist_free (hist_pipcs);
  if (control->stream_type == STREAM_TYPE_AUDIO && VDEMUXER_AUDIO_BENCHMARK)
    rt_mon_hist_free (hist_pipcs);
#endif
#endif

#if RTMON_DSI_BENCHMARK
//  rt_mon_list_dump (list);
  if (list)
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

  /* close import plugin */
  if (control->import_close (&control->plugin_ctrl))
    LOG_Error ("Closing import plugin failed.");
  else
    LOGdL (DEBUG_IMPORT, "Import plugin closed.");


shutdown_work_loop:
  /* signal receiver we are at EOS */
  send_eos (control->socket, p, count++, !is_last_packet_commited);

  /* indicate we're down */
  LOGdL (DEBUG_THREAD, "shutdown work_thread done.");
  l4semaphore_up (&control->running_sem);
}




/*****************************************************************************/
/**
* \brief Start sending work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* Connects sockets and unlocks a mutex. So we let the work_thread run.
*/
/*****************************************************************************/
int
sender_start (control_struct_t * control, dsi_socket_ref_t * local,
	      dsi_socket_ref_t * remote)
{
  dsi_socket_t *s;
  int ret;

  /* lock to ensure we are not just creating the work_thread */
  l4semaphore_down (&control->create_sem);

  /* verbose */
  LOGdL (DEBUG_STREAM, "connected.");

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor (local->socket, &s);
  if (ret)
  {
    LOG_Error ("invalid socket");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }

  /* connect socket */
  ret = dsi_socket_connect (s, remote);
  if (ret)
  {
    LOG_Error ("connect failed");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }

  /* start work thread */
  l4semaphore_up (&control->start_sem);

  /* unlock */
  l4semaphore_up (&control->create_sem);
  /* done */
  return 0;
}


/*****************************************************************************/
/**
* \brief Open sender socket
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param socketref socket reference for created DSI-socket
*
* Attach dataspaces to Demuxer-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
int
sender_open_socket (control_struct_t * control,
		    const l4dm_dataspace_t * ctrl_ds,
		    const l4dm_dataspace_t * data_ds,
		    dsi_socket_ref_t * socketref)
{
  int ret;
  l4_threadid_t work_id, sync_id;
  dsi_jcp_stream_t jcp;
  dsi_stream_cfg_t cfg;
  l4_uint32_t flags;
#if BUILD_RT_SUPPORT
  l4_threadid_t pager;
  l4_umword_t word1;
#endif

  /* lock to ensure we are not just creating the work_thread */
  l4semaphore_down (&control->create_sem);

  /* reset control->start_sem to default value ! */
  control->start_sem = L4SEMAPHORE_LOCKED;

  /* what type of socket? */
#if BUILD_RT_SUPPORT
  flags = DSI_SOCKET_SEND;
#else
  flags = DSI_SOCKET_SEND | DSI_SOCKET_BLOCK;
#endif

  /* we don't want to shutdown before we've done anything */
  control->shutdown_work_thread = 0;


  /* start work thread */
  ret = l4thread_create_long (L4THREAD_INVALID_ID, work_loop, ".worker",
			      L4THREAD_INVALID_SP, L4THREAD_DEFAULT_SIZE,
			      L4THREAD_DEFAULT_PRIO, control,
			      L4THREAD_CREATE_ASYNC);
  if (ret < 0)
  {
    Panic ("start work thread failed!");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }
  work_id = l4thread_l4_id (ret);

#if BUILD_RT_SUPPORT
  control->work_thread_id = work_id;

  /* start preempter thread */
  ret =
    l4thread_create_long (L4THREAD_INVALID_ID, preempter_thread, ".preempt",
			  L4THREAD_INVALID_SP, L4THREAD_DEFAULT_SIZE,
			  L4THREAD_DEFAULT_PRIO, control,
			  L4THREAD_CREATE_ASYNC);
  if (ret < 0)
  {
    Panic ("start preempter thread failed!");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }
  control->preempter_id = l4thread_l4_id (ret);

  /* Setting priority for preempter */
  rmgr_set_prio (control->preempter_id, 255);

  /* set preempter */
  pager = L4_INVALID_ID;
  l4_thread_ex_regs_flags(work_id, -1, -1,
		          &control->preempter_id, &pager,
			  &word1, &word1, &word1, L4_THREAD_EX_REGS_NO_CANCEL);
#endif

  /* create socket */
  cfg.num_packets = PACKETS_IN_BUFFER;
  cfg.max_sg = 2;
  sync_id = L4_INVALID_ID;
  ret =
    dsi_socket_create (jcp, cfg, (l4dm_dataspace_t *) ctrl_ds,
		       (l4dm_dataspace_t *) data_ds, work_id, &sync_id, flags,
		       &control->socket);
  if (ret)
  {
    Panic ("create DSI socket failed");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }

  /* get socket reference */
  ret = dsi_socket_get_ref (control->socket, socketref);
  if (ret)
  {
    Panic ("get socket ref failed");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }

  /* unlock */
  l4semaphore_up (&control->create_sem);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
* \brief Closes sender socket
* 
* The DSI-Socket is closed after the work_thread is stoped.
*/
/*****************************************************************************/
int
sender_close (control_struct_t * control, l4_int32_t close_socket_flag)
{
  int ret = 0;

  /* lock to ensure we are not just creating the work_thread */
  l4semaphore_down (&control->create_sem);

  /* signal work_thread to stop */
  control->shutdown_work_thread = 1;

  /* start work thread - ensure work thread can run at least once
   * this avoids starvation if receiver is never ready or not avail
   */
  l4semaphore_up (&control->start_sem);

  /* wait for work_thread shutdown */
  l4semaphore_down (&control->running_sem);

#if BUILD_RT_SUPPORT
  LOGdL (DEBUG_RT_INIT && DEBUG_DEMUXER, "Waiting for preempter shutdown...");

  /* wait for preempt_thread shutdown */
  l4semaphore_down (&control->running_preempt);
  LOGdL (DEBUG_RT_INIT && DEBUG_DEMUXER, "preempter shut down.");
#endif

  /* close socket */
  if (close_socket_flag)
  {
    ret = dsi_socket_close (control->socket);
    if (ret)
      LOG_Error ("close socket failed (%x)", -ret);
  }

#if BUILD_RT_SUPPORT
  /* wait for preempt_thread shutdown */
  l4semaphore_up (&control->running_preempt);
#endif

  /* signal that we can run the next work_thread */
  l4semaphore_up (&control->running_sem);

  /* unlock */
  l4semaphore_up (&control->create_sem);

  /* done */
  return ret;
}
