/*
 * \brief   Work thread w/ workloop for VERNER's core component
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

#include <l4/util/l4_macros.h>

#if BUILD_RT_SUPPORT
/* rt support */
#include <l4/sys/rt_sched.h> // RT scheduling
#include <l4/util/atomic.h>
#include <l4/rmgr/librmgr.h> // for rmgr_set_prio
#include <l4/util/kip.h> // l4util_kip_map
#if RT_USE_CPU_RESERVE
#include <l4/cpu_reserve/sched.h>
#endif
#endif // RT support

/* include work_loop functions */
#include "work_loop_audio.h"
#include "work_loop_video.h"

#if VCORE_VIDEO_BENCHMARK || VCORE_AUDIO_BENCHMARK
/* benchmark */
/* rt_mon stuff */
#include <l4/rt_mon/histogram.h>

rt_mon_histogram_t *hist_pipcs = NULL;

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
  l4util_micros2l4to (1000000, &man, &exp);	/* microsec per period */

  LOGdL (DEBUG_RT_PREEMPT && (DEBUG_VCORE || DEBUG_ACORE), 
      "Waiting for Preemption-IPCs from " l4util_idfmt,
      l4util_idstr(control->work_thread_id));

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
      /* if type == Timeslive_overrun (1) for mandatory (id 1) or
       * type == any for optional (id 2):
       *   increase preemption count */
      unsigned int type = (word2 & 0x80000000U) >> 31;
      unsigned int id = (word2 & 0x7f000000U) >> 24;
      if (control->stream_type == STREAM_TYPE_VIDEO) // && (id == 2))
      {
	// we do want to count ALL preemptions
	l4util_inc32 (&control->rt_preempt_count);
      }
#if !(DEBUG_RT_PREEMPT && (DEBUG_VCORE || DEBUG_ACORE))
      if (control->rt_verbose_pipc != 0)
      {
#endif
	LOG ("%s: received P-IPC (Type:%i (%s) ID:%u, Time:%llu)",
	     control->stream_type == STREAM_TYPE_VIDEO ? "(V)" : "(A)",
	     type,
	     type ? "TS overrun" : "DL miss",
	     id,
	     ((unsigned long long) word2 << 32 | word1) &
	     0xffffffffffffffULL);
#if !(DEBUG_RT_PREEMPT && (DEBUG_VCORE || DEBUG_ACORE))
      }
#endif
    }
  }
  /* indicate we're down */
  l4semaphore_up (&control->running_preempt);

  LOGdL (DEBUG_RT_PREEMPT && (DEBUG_VCORE || DEBUG_ACORE), 
      "preemption thread killed");
}
#endif

/*****************************************************************************/
/**
 * \brief lock Work thread creation.
 *
 * lock to ensure we don't create two work_threads or sending signals while
 * creating.
 */
/*****************************************************************************/
void
work_loop_lock (control_struct_t * control)
{
  /* lock to ensure we don't create two work_threads */
  l4semaphore_down (&control->create_sem);
}

/*****************************************************************************/
/**
 * \brief unlock Work thread creation.
 *
 * unlock to ensure we can create two work_threads or sending signals.
 */
/*****************************************************************************/
void
work_loop_unlock (control_struct_t * control)
{
  /* unlock */
  l4semaphore_up (&control->create_sem);
}

/*****************************************************************************/
/**
 * \brief Create work thread if it doesn't exists yet.
 *
 */
/*****************************************************************************/
int
work_loop_create (control_struct_t * control)
{
#if BUILD_RT_SUPPORT
  l4_threadid_t preempter, pager;
  l4_umword_t word1;
  int ret;
#endif

  /* we don't want to shutdown before we've done anything */
  control->shutdown_work_thread = 0;

  /* start work thread if it doesn't exists yet */
  if (control->work_thread < 0)
  {
    /* reset control->start_control->start_send_sem and control->start_recv_sem to default values ! */
    control->start_send_sem = L4SEMAPHORE_LOCKED;
    control->start_recv_sem = L4SEMAPHORE_LOCKED;

    /* start work thread if it doesn't exists yet */
    if (control->stream_type == STREAM_TYPE_VIDEO)
      control->work_thread =
	l4thread_create_long (L4THREAD_INVALID_ID, work_loop_video, ".vworker",
	    L4THREAD_INVALID_SP, L4THREAD_DEFAULT_SIZE,
	    L4THREAD_DEFAULT_PRIO, control,
	    L4THREAD_CREATE_ASYNC);
    else
      control->work_thread =
	l4thread_create_long (L4THREAD_INVALID_ID, work_loop_audio, ".aworker",
	    L4THREAD_INVALID_SP, L4THREAD_DEFAULT_SIZE,
	    L4THREAD_DEFAULT_PRIO, control,
	    L4THREAD_CREATE_ASYNC);

#if BUILD_RT_SUPPORT
    control->work_thread_id = l4thread_l4_id (control->work_thread);

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
    l4_thread_ex_regs (control->work_thread_id, -1, -1,
		       &preempter, &pager, &word1, &word1, &word1);
#endif
  }

  return control->work_thread;
}

/*****************************************************************************/
/**
* \brief Start receiving work thread
*
* Unlocks a mutex. So we let the work_thread run.
*/
/*****************************************************************************/
int
work_loop_start_receiver (control_struct_t * control)
{
  /* start work thread */
  l4semaphore_up (&control->start_recv_sem);
  /* done */
  return 0;
}


/*****************************************************************************/
/**
* \brief Start sending work thread
*
* Unlocks a mutex. So we let the work_thread run.
*/
/*****************************************************************************/
int
work_loop_start_sender (control_struct_t * control)
{
  /* start work thread */
  l4semaphore_up (&control->start_send_sem);
  /* done */
  return 0;
}

/*****************************************************************************/
/**
* \brief Stop work thread
*
* Signals work_thread to exit
*/
/*****************************************************************************/
int
work_loop_stop (control_struct_t * control)
{
  /* signal work_thread to stop */
  control->shutdown_work_thread = 1;

  /* start work thread - ensure work thread can run at least once
   * this avoids starvation if receiver is never ready or not avail
   */
  work_loop_start_receiver (control);
  work_loop_start_sender (control);

  /* wait for work_thread shutdown */
  l4semaphore_down (&control->running_sem);

#if BUILD_RT_SUPPORT
  LOGdL (DEBUG_RT_INIT && (DEBUG_VCORE || DEBUG_ACORE), 
      "Waiting for preempter shutdown...");
  /* wait for preempt_thread shutdown */
  l4semaphore_down (&control->running_preempt);
  LOGdL (DEBUG_RT_INIT && (DEBUG_VCORE || DEBUG_ACORE), 
      "preempter shut down.");
#endif

  /* sockets will be closed due callback function !
   * so we do nothing here :)
   */

#if BUILD_RT_SUPPORT
  /* wait for preempt_thread shutdown */
  l4semaphore_up (&control->running_preempt);
#endif

  /* signal that we can run the next work_thread */
  l4semaphore_up (&control->running_sem);

  /* done */
  return 0;
}

