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
#include "metronome.h"
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
#if RT_USE_CPU_RESERVE
#include <l4/cpu_reserve/sched.h>
#endif
#endif

#if VSYNC_VIDEO_BENCHMARK || VSYNC_AUDIO_BENCHMARK || RTMON_DSI_BENCHMARK
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

#if BUILD_RT_SUPPORT
/*****************************************************************************/
/**
 * \brief Preempter thread.
 *
 * \param data Thread data (unused)
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
  l4util_micros2l4to (1000000, &man, &exp);	/* microsec per period */

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
      /* in synchronizer component we ignore the preemption IPCs */
#if DEBUG_RT_PREEMPT && DEBUG_SYNC_HEAVY
      if (control->rt_verbose_pipc != 0)
      {
	LOG ("%s: received P-IPC (Type:%i (%s) ID:%u, Time:%llu)",
	     control->stream_type == STREAM_TYPE_VIDEO ? "(V)" : "(A)",
	     (word2 & 0x80000000U) >> 31,
	     ((word2 & 0x80000000U) >> 31) == 1 ? "TS overrun" : "DL miss",
	     (word2 & 0x7f000000U) >> 24,
	     ((unsigned long long) word2 << 32 | word1) &
	     0xffffffffffffffULL);
      }
#endif
    }
  }
  /* indicate we're down */
  l4semaphore_up (&control->running_preempt);

  LOGdL (DEBUG_RT_PREEMPT && DEBUG_SYNC, "preemption thread killed");
}
#endif // RT support

/*****************************************************************************/
/**
* \brief Start receiving work thread
* 
* \param local local socket reference
* \param remote remote socket reference
*
* Connects sockets and unlocks a mutex. So we let the work_thread run.
*/
/*****************************************************************************/
int
receiver_start (control_struct_t * control, dsi_socket_ref_t * local,
		dsi_socket_ref_t * remote)
{
  dsi_socket_t *s;
  int ret;

  /* lock to ensure we are not just creating the work_thread */
  l4semaphore_down (&control->create_sem);

  /* do nothing */
  LOGdL (DEBUG_STREAM, "connected.");

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor (local->socket, &s);
  if (ret)
  {
    Panic ("invalid socket");
    /* unlock */
    l4semaphore_up (&control->create_sem);
    return -1;
  }

  /* connect socket */
  ret = dsi_socket_connect (s, remote);
  if (ret)
  {
    Panic ("connect failed");
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
* \brief Open receive socket
* 
* \param ctrl_ds already allocated control dataspace
* \param data_ds already allocated data dataspace
* \param socketref socket reference for created DSI-socket
*
* Attach dataspaces to Sync-Component. The DSI-Socket is created. 
* The work_thread also, but it's waiting for the start_signal.
*/
/*****************************************************************************/
int
receiver_open_socket (control_struct_t * control,
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
  l4_threadid_t preempter, pager;
  l4_umword_t word1;
#endif

  /* lock to ensure we are not just creating the work_thread */
  l4semaphore_down (&control->create_sem);

  /* we don't want to shutdown before we've done anything */
  control->shutdown_work_thread = 0;

  work_id = L4_INVALID_ID;

  /* what type of socket? */
#if BUILD_RT_SUPPORT
  flags = DSI_SOCKET_RECEIVE;
#else
  flags = DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK;
#endif

  /* set semaphore to default */
  control->start_sem = L4SEMAPHORE_LOCKED;

  /* start work thread */
  if (control->stream_type == STREAM_TYPE_VIDEO)
    ret = l4thread_create_long (L4THREAD_INVALID_ID, work_loop_video, ".vworker",
	L4THREAD_INVALID_SP, L4THREAD_DEFAULT_SIZE,
      	L4THREAD_DEFAULT_PRIO, control,
	L4THREAD_CREATE_ASYNC);
  else
    ret = l4thread_create_long (L4THREAD_INVALID_ID, work_loop_audio, ".aworker",
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
  preempter = l4thread_l4_id (ret);

  /* Setting priority for preempter */
  rmgr_set_prio (preempter, 255);

  /* set preemter */
  pager = L4_INVALID_ID;
  l4_thread_ex_regs_flags(work_id, -1, -1,
		          &preempter, &pager, &word1, &word1, &word1,
			  L4_THREAD_EX_REGS_NO_CANCEL);
#endif // RT support

  /* create socket */
  cfg.num_packets = PACKETS_IN_BUFFER;
  cfg.max_sg = 2;
  sync_id = L4_INVALID_ID;
  ret = dsi_socket_create (jcp, cfg, (l4dm_dataspace_t *) ctrl_ds,
			   (l4dm_dataspace_t *) data_ds, work_id, &sync_id,
			   flags, &control->socket);
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
* \brief Closes receive socket
* 
* The DSI-Socket is closed after the work_thread is stoped.
*/
/*****************************************************************************/
int
receiver_close (control_struct_t * control, l4_int32_t close_socket_flag)
{
  int ret = 0;

  /* lock to ensure we are not just creating the work_thread */
  l4semaphore_down (&control->create_sem);

  /* signal work_thread to stop */
  control->shutdown_work_thread = 1;

  /* ensure the work_thread can really run.
   * It could happend that close is called BEFORE start is ever called !
   */
  l4semaphore_up (&control->start_sem);

  /* wait for work_thread shutdown */
  l4semaphore_down (&control->running_sem);

#if BUILD_RT_SUPPORT
  /* wait for preempt_thread shutdown */
  l4semaphore_down (&control->running_preempt);
#endif

  /* close socket */
  if (close_socket_flag)
  {
    ret = dsi_socket_close (control->socket);
    if (ret)
      LOGl ("close socket failed (%x)", -ret);
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

