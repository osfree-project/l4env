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

#ifndef __DEFINES_H_
#define __DEFINES_H_


/* dope */
#include <dopelib.h>

/* env */
#include <stdarg.h>
#include <stdio.h>

/* verner */
#include "arch_globals.h"
#include "verner_config.h"

/* local */
#include "mutex.h"


/* 
 * internal representation for playlist 
 */
typedef struct
{
  int id;			/* id of file */
  int row;			/* row in DOpE's grid */
  char *filename;
  void *next;			/* next entry in list */
  void *prev;			/* prev entry in list */
} pl_entry_t;
extern int pl_entries;
extern pl_entry_t pl_first_elem;	/* first element */
extern pl_entry_t *pl_last_elem;	/* last element */


/* 
 * state of GUI
 */
typedef struct
{
  /* play, stop, paused ?  values are not disjunct, so test with & instead of == */
  unsigned int playmode;
  /* random */
  int random;
  /* repeat */
  int repeat;
  /* text only (not DOpEd) */
  int text_only;
  /* audio muted ? */
  int mute;
  /* lock for event_manager */
  MUTEX *event_lock;
  /* lock for seek bar */
  MUTEX *seek_lock;
  /* info about openend stream */
  stream_info_t stream_info;
  /* current playlist entry played */
  pl_entry_t *current_pl_entry;
  /* just playing something */
  int just_playing;
  /* id of fx plugin */
  int fx_plugin;
  /* use filesize to seek in file */
  int seek_via_filesize;

#if BUILD_RT_SUPPORT
  /* verbose all preemption ipcs ? */
  int rt_verbose_preemption_ipc_demux;
  int rt_verbose_preemption_ipc_core_audio;
  int rt_verbose_preemption_ipc_core_video;
  int rt_verbose_preemption_ipc_sync;
  /* period lenght for all worker threads [microsec] */
  unsigned long rt_period;
  /* 
   * reservation times for verner components
   * time in microsec PER frame/chunk !
   */
  unsigned long rt_reservation_demux_audio;
  unsigned long rt_reservation_demux_video;
  unsigned long rt_reservation_core_audio;
  unsigned long rt_reservation_core_video;
  unsigned long rt_reservation_core_video_pp;
  unsigned long rt_reservation_sync_audio;
  unsigned long rt_reservation_sync_video;
#endif

#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
  int silent;
#endif
} gui_state_t;

extern gui_state_t gui_state;

/*
 * dope's app id, 
 * should be in gui_state but it isn't cause I use gen_dopecmd 
 */
extern long app_id;
/* ensure not using dope, if in textonly mode */
#define vdope_cmd if(!gui_state.text_only)dope_cmd
#define vdope_cmdf if(!gui_state.text_only)dope_cmdf
#define vdope_bind if(!gui_state.text_only)dope_bind
#define vdope_bindf if(!gui_state.text_only)dope_bindf
#define vdope_req if(gui_state.text_only) { result[0]='\0'; } else dope_req



/* events for event manager */
#define  EV_PLAYLIST_ADD	0	/* add stream/file to playlist */
#define  EV_PLAYLIST_REM	1	/* remove stream/file to playlist */
#define  EV_PLAYLIST_CLEAR	2	/* clear playlist */
#define  EV_STOP		3	/* stop playing */
#define  EV_PLAY		4	/* begin playing */
#define  EV_PAUSE		5	/* pause playing */
#define  EV_BWD			6	/* backward in playlist */
#define  EV_FWD			7	/* forward in playlist */
#define  EV_EOF			8	/* end of current stream reached */

/* commands for UI callback and start_playing()*/
#define   CMD_ADD 		0
#define   CMD_CLEAR 		1
#define   CMD_SHOW 		2
#define   CMD_BW		4
#define   CMD_PLAY		5
#define   CMD_STOP		6
#define   CMD_PAUSE		7
#define   CMD_FW		8
#define   CMD_SEEK		9
#define   CMD_VOL		0
#define   CMD_MUTE		10
#define   CMD_RANDOM		11
#define   CMD_REPEAT		12
#define   CMD_FORCE_STREAM	13	/* force playback of given file */
#define   CMD_FIRST_STREAM	14	/* play first file in list */
#define   CMD_NEXT_STREAM	15	/* play next file in list */
#define   CMD_PREV_STREAM	16	/* play prev file in list */
#define   CMD_RAND_STREAM	17	/* play random file from list */
#define   CMD_QAP_TOGGLE	18	/* switch QAP on/off */
#define   CMD_QAP_CHANGE	19	/* change quality level */
#define   CMD_PP_SEND		20	/* send command to postproce engine */
#define   CMD_GOOM_TOGGLE	21	/* switch GOOM on/off */
#define   CMD_RT_TIMES_UPDATE	22	/* update RT-period lenght for all workers */
#define   CMD_RT_VERBOSE_PIPC_TOGGLE_DEMUX	23	/* toggle verbose of preemption ipcs */
#define   CMD_RT_VERBOSE_PIPC_TOGGLE_CORE_AUDIO	24
#define   CMD_RT_VERBOSE_PIPC_TOGGLE_CORE_VIDEO	25
#define   CMD_RT_VERBOSE_PIPC_TOGGLE_SYNC	26
#define   CMD_SPEED_CHANGE	27

static void setInfo (const char *format, ...);
void setPlaymode (int playmode);

/*
 * set info text
 */
#ifndef __setinfo_decl_
#define __setinfo_decl_
static inline void
setInfo (const char *format, ...)
{
  va_list list;
  static char infostr[1024];

  va_start (list, format);
  vsnprintf (infostr, 1024, format, list);
  va_end (list);
  if (gui_state.text_only)
    printf ("VERNER: %s", infostr);
  else
    vdope_cmdf (app_id, "main_label_info.set(-text \"%s\")", infostr);
}
#endif

/*
 * set playmode 
 */
extern inline void
setPlaymode (int playmode)
{
#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
  if (playmode == PLAYMODE_PLAY && gui_state.silent)
    playmode = PLAYMODE_DROP;
#endif
  gui_state.playmode = playmode;
  /* push button for current playmode but not w/o gui */
  if (gui_state.text_only)
    return;
  vdope_cmdf (app_id, "main_btn_play.set(-state %i)",
	      (gui_state.playmode & PLAYMODE_PLAY) > 0);
  vdope_cmdf (app_id, "main_btn_pause.set(-state %i)",
	      (gui_state.playmode & PLAYMODE_PAUSE) > 0);
  vdope_cmdf (app_id, "main_btn_stop.set(-state %i)",
	      (gui_state.playmode & PLAYMODE_STOP) > 0);
}

#if 0				/* dump event received by eventmanager */
#define dumpEvent \
    switch (command) { \
	case  EV_PLAYLIST_ADD:	LOG("event: add stream/file to playlist"); break; \
	case  EV_PLAYLIST_REM:	LOG("event: remove stream/file to playlist"); break; \
	case  EV_PLAYLIST_CLEAR:LOG("event: clear playlist"); break; \
	case  EV_STOP:		LOG("event: stop playing"); break; \
	case  EV_PLAY:		LOG("event: begin playing"); break; \
	case  EV_PAUSE:		LOG("event: pause playing"); break; \
	case  EV_BWD:		LOG("event: backward in playlist"); break; \
	case  EV_FWD:		LOG("event: forward in playlist"); break; \
	case  EV_EOF:		LOG("event: end of current stream reached"); break; \
	default: LOG("event: unknown"); }
#else
#define dumpEvent ;
#endif




#endif
