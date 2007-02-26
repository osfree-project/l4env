/*
 * \brief   QAP for VERNER's core component
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

#include "qap.h"
#include "arch_globals.h"

#if VCORE_VIDEO_ENABLE_QAP

#if VCORE_VIDEO_BENCHMARK || VCORE_AUDIO_BENCHMARK 
#include <l4/rt_mon/histogram.h>

extern rt_mon_histogram_t *hist_pipcs;
#endif

/*
 * QAP (video only):
 * calculate new quality level to play video without framedrops,
 * but with highest quality possible.
 *
 * in RT-Mode we use the received preemption IPCs 
 * in NON-RT-Mode we use the buffer calc function
 */
inline void
QAP (control_struct_t * control)
{
  /* in RT mode we count the preemption IPCs */
  if (control->rt_preempt_count >
      (RT_CORE_VIDEO_QAP_ALLOWED_PREEMPTIONS_MAX *
       VCORE_VIDEO_QAP_CHECKPOINT) / 100)
  {
    /* if we had a lot of preemption IPCs, we decrease QAP */
    if (control->plugin_ctrl.currentQLevel > control->plugin_ctrl.minQLevel)
      control->plugin_ctrl.currentQLevel--;
  }
  else if (control->rt_preempt_count <
	   (RT_CORE_VIDEO_QAP_ALLOWED_PREEMPTIONS_MIN *
	    VCORE_VIDEO_QAP_CHECKPOINT) / 100)
  {
    /* if there have been few preemption IPCs in a while, increase */
    if (control->plugin_ctrl.currentQLevel < control->plugin_ctrl.maxQLevel)
      control->plugin_ctrl.currentQLevel++;
  }
  /* if there have been a some preemption IPCs (between MIN and MAX), don't do anything */
  LOGdL (DEBUG_RT_PREEMPT && DEBUG_VCORE, "Preemption Counter #: '%d'", control->rt_preempt_count);
  /* reset counter */
#if VCORE_VIDEO_BENCHMARK || VCORE_AUDIO_BENCHMARK
  rt_mon_hist_insert_data(hist_pipcs, control->rt_preempt_count, 1);
#endif
  control->rt_preempt_count = 0;
}

#else /* VCORE_VIDEO_ENABLE_QAP disabled */

inline void
QAP (control_struct_t * control)
{
  /* nothing to do without QAP */
}
#endif

/* end QAP */
