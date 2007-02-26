/**
 * \brief   Receive functions for VERNER's sync component
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

#ifndef _RECEIVE_H_
#define _RECEIVE_H_

/* l4 */
#include <l4/sys/types.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* verner */
#include "arch_globals.h"

/* this is the structure of all variables, flag, ... each separate work_thread needs */
typedef struct
{
  /* currently in use ! */
  int in_use;

  /* the socket to receive from */
  dsi_socket_t *socket;

  /* the semaphore to start our work thread */
  l4semaphore_t start_sem;
  /* if work thread is running */
  l4semaphore_t running_sem;
  /* if we're just creating our work thread */
  l4semaphore_t create_sem;

  /* flag indicating we should shutdown our work_thread */
  int shutdown_work_thread;

  /* requested stream type STREAM_TYPE_VIDEO | _AUDIO */
  int stream_type;
  /* info about streams */
  stream_info_t streaminfo;

  /* ctrl struct to control export plugin */
  plugin_ctrl_t plugin_ctrl;
  /* function ptr to call plugin */
  int (*out_init) (plugin_ctrl_t * attr, stream_info_t * info);
  int (*out_commit) (plugin_ctrl_t * attr);
  int (*out_step) (plugin_ctrl_t * attr, void *addr);
  int (*out_close) (plugin_ctrl_t * attr);
  int (*out_getPosition) (plugin_ctrl_t * attr, double *position);
  int (*out_setVolume) (plugin_ctrl_t * attr, int left, int right);
  /* is the output open? */
  int out_open;

  /* id of current plugin */
  int fx_plugin_id;
  /* ctrl struct to control fx plugin */
  plugin_ctrl_t fx_plugin_ctrl;
  /* function ptr to call plugin */
  int (*fx_init) (plugin_ctrl_t * attr, stream_info_t * info, char *options);
  int (*fx_step) (plugin_ctrl_t * attr, unsigned char *buffer);
  int (*fx_close) (plugin_ctrl_t * attr);

  /* volume settings (audio part only) */
  int volume_right;
  int volume_left;

#if BUILD_RT_SUPPORT
  /* rt support */
  /* if preempter thread is running */
  l4semaphore_t running_preempt;
  /* thread_id of work thread */
  l4_threadid_t work_thread_id;
  /* next period id */
  l4_threadid_t next_period_id;
  /* the lenght of one period in µsec */
  unsigned long rt_period;
  /* reservation time in microsec */
  unsigned long rt_reservation;
  /* verbose received preemption ipcs */
  int rt_verbose_pipc;
#endif

} control_struct_t;


int receiver_open_socket (control_struct_t * control,
			  const l4dm_dataspace_t * ctrl_ds,
			  const l4dm_dataspace_t * data_ds,
			  dsi_socket_ref_t * socketref);

int receiver_start (control_struct_t * control, dsi_socket_ref_t * local,
		    dsi_socket_ref_t * remote);

int receiver_close (control_struct_t * control, l4_int32_t close_socket_flag);

void work_loop_audio (void * data);
void work_loop_video (void * data);

#endif
