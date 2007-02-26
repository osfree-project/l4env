/*
 * \brief   Type definitions for VERNER's demuxer component
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

#ifndef _VDEMUXER_TYPES_H_
#define _VDEMUXER_TYPES_H_

#include <l4/sys/types.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

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
  /* plugin id "-1"==Autodetect */
  int plugin_id;

  /* ctrl struct to control plugin */
  plugin_ctrl_t plugin_ctrl;
  /* function ptr to call plugin */
  int (*import_init) (plugin_ctrl_t * attr, stream_info_t * info);
  int (*import_commit) (plugin_ctrl_t * attr);
  int (*import_step) (plugin_ctrl_t * attr, void *addr);
  int (*import_close) (plugin_ctrl_t * attr);
  int (*import_seek) (plugin_ctrl_t * attr, void *addr, double position,
		      int whence);

  /* got seek cmd */
  int seek;
  /* relative or absolute ? */
  int seek_whence;
  /* positions to seek to */
  double seek_position;
  /* fileoffset to seek to */
  long seek_fileoffset;

#if BUILD_RT_SUPPORT
  /* rt support */
  /* if preempter thread is running */
  l4semaphore_t running_preempt;
  /* thread_id of work thread */
  l4_threadid_t work_thread_id;
  /* thread_id of preempter thread */
  l4_threadid_t preempter_id;
  /* counting the preemptions per period */
  unsigned int rt_preempt_count;
  /* period in microsec */
  unsigned long rt_period;
  /* reservation time in microsec */
  unsigned long rt_reservation;
  /* verbose received preemption ipcs */
  int rt_verbose_pipc;
#endif


} control_struct_t;

#endif
