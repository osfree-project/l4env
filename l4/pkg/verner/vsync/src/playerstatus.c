/*
 * \brief   Controls playerstatus for VERNER's sync component
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


/* STD/L4 Includes */
#include <stdio.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

/* verner */
#include "arch_globals.h"

/* local includes */
#include "mutex.h"
#include "timer.h"
#include "metronome.h"
#include "playerstatus.h"

/* configuration */
#include "verner_config.h"

/* 
 * the semaphore to control access to playerstatus
 * because it can be changed from too many threads 
 */
static l4semaphore_t set_player_modus_save = L4SEMAPHORE_UNLOCKED;

#if BUILD_RT_SUPPORT
/* rt support */
#include <l4/sys/rt_sched.h>
#else
/* clients waiting for these semaphores */
static MUTEX *modus_wait_audio = NULL;
static MUTEX *modus_wait_video = NULL;
#endif

/* we start in throw-away mode */
static int current_player_modus = PLAYMODE_DROP;
static int last_player_modus = PLAYMODE_DROP;

/* 
 * the system time when video was pause
 * it's necessary to measure how long it's pause for video w/o audio
 * and to tell metronme to adjust it's global time
 */
static unsigned long ul_msec_paused_time;


void
init_playerstatus (int streamtype)
{
#if !BUILD_RT_SUPPORT
  /* now we lock it */
  l4semaphore_down (&set_player_modus_save);

  /* create audio mutex */
  if ((streamtype == STREAM_TYPE_AUDIO) && (modus_wait_audio == NULL))
    modus_wait_audio = create_mutex (MUTEX_LOCKED);
  /* create video mutex */
  if ((streamtype == STREAM_TYPE_VIDEO) && (modus_wait_video == NULL))
    modus_wait_video = create_mutex (MUTEX_LOCKED);

  /* unlock */
  l4semaphore_up (&set_player_modus_save);
#endif
}

void
close_playerstatus (int streamtype)
{
#if BUILD_RT_SUPPORT
  /* in rt mode we've nothing to do */
#else
  /* now we lock it */
  l4semaphore_down (&set_player_modus_save);

  /* unlock all waiting clients */
  if (modus_wait_audio != NULL)
    mutex_unlock (modus_wait_audio);
  if (modus_wait_video != NULL)
    mutex_unlock (modus_wait_video);

  /* close audio mutex */
  if ((streamtype == STREAM_TYPE_AUDIO) && (modus_wait_audio != NULL))
    destroy_mutex (modus_wait_audio);

  /* close video mutex */
  if ((streamtype == STREAM_TYPE_VIDEO) && (modus_wait_video != NULL))
    destroy_mutex (modus_wait_video);

  /* avoid reuse */
  modus_wait_audio = NULL;
  modus_wait_video = NULL;

  /* unlock */
  l4semaphore_up (&set_player_modus_save);
#endif
}


/* set player mode */
void
set_player_mode (int new_modus)
{
  /* ignore already set */
  if (new_modus == current_player_modus)
    return;

  /* now we lock it */
  l4semaphore_down (&set_player_modus_save);

  /*
   * accecpt only valid changes:
   * PAUSE <--> {PLAY, DROP}
   * yes, currently it's the only valid, but keep FF, STOP and so on in mind 
   */
  switch (current_player_modus)
  {
  case PLAYMODE_PLAY:
    if (new_modus == PLAYMODE_PAUSE)
    {
      /* rember time when pause begun */
      ul_msec_paused_time = get_time_millisec ();
      break;
    }
    goto mutex_down;
    break;
  case PLAYMODE_DROP:
    break;
  case PLAYMODE_PAUSE:
    if (new_modus == PLAYMODE_PLAY)
    {
      /* adjust metronome to recalculate it's ul_msec_video_start_time */
      metronome_adjust_video_start_time (get_diff
					 (ul_msec_paused_time,
					  get_time_millisec ()));
      break;
    }
    else if (new_modus == PLAYMODE_DROP)
      break;
    goto mutex_down;
    break;
  default:
    goto mutex_down;
  }

  /* if we're back in play from ff */
  last_player_modus = current_player_modus;
  current_player_modus = new_modus;

#if !BUILD_RT_SUPPORT
  /* enable waiting clients (audio + video) */
  if (modus_wait_audio != NULL)
    mutex_unlock (modus_wait_audio);
  if (modus_wait_video != NULL)
    mutex_unlock (modus_wait_video);
#endif

mutex_down:
  /* unlock */
  l4semaphore_up (&set_player_modus_save);

}


/*
 * check player mode:
 * wait's until mode. if mode = 0 it's not waiting
 * streamtype is AUDIO|VIDEO ...
 * return: current mode (after waiting)
 */
inline int
check_player_mode (int wait_until_modi, int streamtype)
{
#if BUILD_RT_SUPPORT
  if (!(current_player_modus & wait_until_modi))
    return -1;
#else
  while (!(current_player_modus & wait_until_modi))
  {
    /* we wait only for video and audio */
    if ((streamtype == STREAM_TYPE_VIDEO) && (modus_wait_video != NULL))
      mutex_lock (modus_wait_video);
    else if ((streamtype == STREAM_TYPE_AUDIO) && (modus_wait_audio != NULL))
      mutex_lock (modus_wait_audio);
  }
#endif
  return current_player_modus;
}

