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
  /* in non-RT mode we use the buffer calc function */
  int dsi_filllevel;

  switch (control->plugin_ctrl.mode)
  {
  case PLUG_MODE_DEC:
    /* get buffer size and and it's fill level from EXPORT */
    dsi_filllevel =
      dsi_socket_get_num_committed_packets (control->send_socket) * 100 /
      dsi_socket_get_packet_num (control->send_socket);

    /* calculate new QLevel - >90% Q++  */
    if ((dsi_filllevel > 90) && (control->plugin_ctrl.currentQLevel < control->plugin_ctrl.maxQLevel))	/* */
      control->plugin_ctrl.currentQLevel++;
    /* <50% Q-- */
    else if ((dsi_filllevel < 50)
	     && (control->plugin_ctrl.currentQLevel >
		 control->plugin_ctrl.minQLevel))
      control->plugin_ctrl.currentQLevel--;

    break;
  case PLUG_MODE_ENC:
    /* get buffer size and and it's fill level from IMPORT */
    dsi_filllevel =
      dsi_socket_get_num_committed_packets (control->recv_socket) * 100 /
      dsi_socket_get_packet_num (control->recv_socket);

    /* calculate new QLevel - <10% Q++ */
    if ((dsi_filllevel < 10) && (control->plugin_ctrl.currentQLevel < control->plugin_ctrl.maxQLevel))	/* */
      control->plugin_ctrl.currentQLevel++;
    /* >50% Q-- */
    else if ((dsi_filllevel > 50)
	     && (control->plugin_ctrl.currentQLevel >
		 control->plugin_ctrl.minQLevel))
      control->plugin_ctrl.currentQLevel--;

    break;
  }
}

#else /* VCORE_VIDEO_ENABLE_QAP disabled */

inline void
QAP (control_struct_t * control)
{
  /* nothing to do without QAP */
}
#endif

/* end QAP */
