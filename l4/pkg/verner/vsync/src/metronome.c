/*
 * \brief   Metronome for VERNER's sync component
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

/*  METRONOME uses parts and ideas of GPL'ed DOpE */

/* verner */
#include "arch_globals.h"

/* local */
#include "timer.h"
#include "metronome.h"
#include "work_loop.h"

/* configuration */
#include "verner_config.h"

/* 
 *
 * BIG FAT README BEFORE TRYING TO UNDERSTAND:
 * Metronome uses MILLISEC but also MICROSEC for higher precision !!
 * So take care to distinguish between.
 *
 */

/* FIXME - XXX - TODO:
 * THIS IS NOT HANDLE-SAFE - ONLY ONE video per sync component is possible
 */

/*
 * 탎ec values
 */

/* the lenght of one frame in 탎ec */
static unsigned long ul_usec_frame_period;
/* start time of current frame (탎ec)  */
static unsigned long ul_usec_frame_start_time;

/*
 * msec values
 */

/*
 * the current time in msec 
 * either it's the audio_position or the video since video_start_time
 */
static double dbl_msec_current_time;

/* the system time when video start in msec */
static unsigned long ul_msec_video_start_time;

/* last sync point set be demuxer/core */
static double dbl_last_video_sync_pts = 00.00;

/*
 * flags, counter 
 */

/* noof frames until next sync check */
static int int_frames_until_check = 0;
/* noof frames to hard drop */
static int int_frames2drop = 0;
/* the A/V-async in 탎ec */
signed long sl_usec_av_async = 0;

/*
 * audio support
 */

/* true if we have an active audio thread to synchronize with */
static int flag_audio_as_time_src = 0;
/* plugin_ctrl for call of ao_getPosition */
static plugin_ctrl_t *audio_attr;
/* function to get audio position */
static int (*ao_getPosition) (plugin_ctrl_t * attr, double *position) = NULL;

/* 
 * times to calculate pc-soundcard clock drift 
 */
/* last system time */
static double dbl_usec_last_system_time = 0;
/* last audio time */
static double dbl_usec_last_audio_position = 0;
/* the correction value to compensate soundcard/pc clockdrift */
static double dbl_clock_drift_factor = 1;
/* should metronome calculate clockdrift ? */
static int flag_dont_calculate_clockdrift = 0;
/* the semaphore to protect hardware access */
static l4semaphore_t sem_audio = L4SEMAPHORE_UNLOCKED;

#if VSYNC_WAIT_TIME_BENCHMARK
#include <l4/rt_mon/event_list.h>
/* list to store wait time difference */
extern rt_mon_event_list_t* list_wait;
#endif

/*
 * internal helper functions 
 */
inline static int metronome_check_sync (frame_ctrl_t * frameattr);
inline static void metronome_calculate_soundcard_system_clockdrift (double
								    dbl_usec_system_time,
								    double
								    dbl_usec_audio_position,
								    double
								    dbl_framerate);


/*
 * init metronome (called by video and/or audio worker) 
 */
void
metronome_init (control_struct_t * control)
{
  /* audio thread */
  if (control->streaminfo.type == STREAM_TYPE_AUDIO)
  {
    /* wait if sombody just uses audio pt */
    l4semaphore_down (&sem_audio);

    /* remember for calling ao_getPosition */
    audio_attr = &control->plugin_ctrl;
    ao_getPosition = control->out_getPosition;
    if ((audio_attr) && (ao_getPosition))
      /* indicate we've got an audio worker */
      flag_audio_as_time_src = 1;
    else
      LOG_Error ("can't use audio to sync");

    /* end wait if sombody just uses audio pt */
    l4semaphore_up (&sem_audio);
  }

  /* video thread */
  if (control->streaminfo.type == STREAM_TYPE_VIDEO)
  {
    /* last sync frame - we haven't any yet */
    dbl_last_video_sync_pts = 0;
    /* the A/V-async in 탎ec */
    sl_usec_av_async = 0;
    /* calculate time to sleep per frame in microsec */
    ul_usec_frame_period =
      (unsigned long) (double) (1000000 / control->streaminfo.vi.framerate) *
      dbl_clock_drift_factor;
    /* remember beginning of first frame */
    ul_usec_frame_start_time = get_time_microsec ();
    ul_msec_video_start_time = get_time_millisec ();
  }

  /* pleases metronome to calculate clockdrift */
  flag_dont_calculate_clockdrift = 0;
  /* last system time */
  dbl_usec_last_system_time = 0;
  /* last audio time */
  dbl_usec_last_audio_position = 0;
  /* the correction value to compensate soundcard<-->pc clockdrift */
  dbl_clock_drift_factor = 1;
  /* current time */
  dbl_msec_current_time = 0;

  /* force sync-(re)check */
  int_frames_until_check = int_frames2drop = 0;

  /* verbose */
  LOGdL (DEBUG_SYNC, "Metronome initialized (%s).",
	 (control->streaminfo.type ==
	  STREAM_TYPE_VIDEO) ? "video pt" : "audio pt.");
}

/*
 * it's necessary to measure how long it's pause for video w/o audio
 * and to tell metronme to adjust it's video start time to avoid too
 * fast playback
 */
void
metronome_adjust_video_start_time (unsigned long delay)
{
  LOGdL (DEBUG_SYNC, "adjusting start time by %lu msec", delay);
  /* check sync now */
  int_frames_until_check = 0;
  /* add offset to video start time */
  ul_msec_video_start_time += delay;
}

/*
 * 1) waiting to display frames w/ it's framerate
 * 2) if(frames%SYNC_CHECK_NTH_FRAME == 0) check if video and audio is synchron
 */
inline int
metronome_wait_sync (frame_ctrl_t * frameattr, control_struct_t * control)
{
  /* time used for displaying last frame (탎ec) */
  unsigned long ul_usec_used_time_per_frame;
  /* time to wait (탎ec) */
  signed long sl_usec_wait_time;
#if VSYNC_WAIT_TIME_BENCHMARK
  /* event to store wait time */
  rt_mon_basic_event_t be;
  /* time stamps for measurement */
  rt_mon_time_t wait_start, wait_end;
#endif

  /* check for reset sync frame (may be start or just seeked) 
   * but results are ignored, because we calculate pts mostly out of frameID!
   * and frameID does NOT start at zero after seeking. 
   */
  if (is_reset_sync_point (frameattr->keyframe))
  {
    LOGdL (DEBUG_SYNC, "got reset_sync_point for video at %i msec",
	   (int) frameattr->last_sync_pts);
    /* remember jump offset */
    dbl_last_video_sync_pts = frameattr->last_sync_pts;
    /* check sync now */
    int_frames_until_check = 0;
    /* don't wait with sync - immediatly start metronome_check_sync() */
    int_frames2drop = -1;
    /* set video start time to zero */
    ul_msec_video_start_time = get_time_millisec ();
  }
  /*
   * check if we've got a reconfigure command when stream data has changed 
   */
  if (is_reconfigure_point (frameattr->keyframe))
  {
    LOGdL (DEBUG_SYNC, "reconfiguring");
    /* recalculate some times */
    ul_usec_frame_period =
      (unsigned long) (double) (1000000 / frameattr->vi.framerate) *
      dbl_clock_drift_factor;
    /* recalculate clock drift */
    flag_dont_calculate_clockdrift = 0;
  }

  /* 
   * we don't drop frames, so we do a normal wait
   * we check only every N-th frame if we're out of sync, 
   * else we do just wait the precalculated time 
   */
  if (int_frames2drop == 0)
  {

    /* check time for last frame loop, then we deceide how long to wait */
    ul_usec_used_time_per_frame =
      get_diff (ul_usec_frame_start_time, get_time_microsec ());

    /* add time consumed in out_step function */
    ul_usec_used_time_per_frame += control->plugin_ctrl.step_time_us;

    /* calculate time to wait */
    sl_usec_wait_time = ul_usec_frame_period - ul_usec_used_time_per_frame;

    /* now wait, if we've to wait */
    if (sl_usec_wait_time > 0)
    {
      /* if we have sl_usec_av_async>0 in means we're too slow, so we speed it up */
      if (sl_usec_av_async > 0)
      {
	LOGdL (DEBUG_SYNC_HEAVY, "async=%ldusec (too slow)", sl_usec_av_async);
	sl_usec_av_async = sl_usec_av_async - sl_usec_wait_time;
	sl_usec_wait_time = -sl_usec_av_async;
      }
      else if (sl_usec_av_async < 0)
      {
	LOGdL (DEBUG_SYNC_HEAVY, "async=%ldusec (too fast)", sl_usec_av_async);
	sl_usec_wait_time += -sl_usec_av_async;
	sl_usec_av_async = 0;
/*	sl_usec_av_async = sl_usec_av_async - sl_usec_wait_time;
	sl_usec_wait_time = -sl_usec_av_async;*/
      }

      /* still some 탎ec to wait ? */
      if (sl_usec_wait_time > 0)
      {
#if DEBUG_SYNC_HEAVY
  	printf ("now sleep=%i (used=%i period=%i) [usec]\n",
	    (int) sl_usec_wait_time,
	    (int) ul_usec_used_time_per_frame,
	    (int) ul_usec_frame_period);
#else
	LOGdL (DEBUG_RT_SLEEP && DEBUG_SYNC, "sleeping to sync %ld usecs", 
	    sl_usec_wait_time);
#endif
#if VSYNC_WAIT_TIME_BENCHMARK
 	wait_start = rt_mon_list_measure(list_wait);
#endif
	l4thread_usleep (sl_usec_wait_time);
#if VSYNC_WAIT_TIME_BENCHMARK
  	wait_end = rt_mon_list_measure(list_wait);
 	be.id = 1;
 	be.time = wait_end - wait_start - sl_usec_wait_time;
 	rt_mon_list_insert(list_wait, &be);
#endif
      }
    }
  }
  /* rememeber start time for next frame */
  ul_usec_frame_start_time = get_time_microsec ();

  /* decrease noof frames until next sync_check */
  int_frames_until_check--;

  /*
   * we're still dropping frames ?
   * Then we don't calculate anything, to drop as fast as possible
   */
  if (int_frames2drop > 0)
  {
//    LOGdL (DEBUG_SYNC_HEAVY, "dropping one :) of %d", int_frames2drop);
    int_frames2drop--;
  }
  else
    /* should we already check sync ? */
    if (int_frames_until_check <= 0)
    {
//      LOGdL (DEBUG_SYNC_HEAVY, "Checking sync");
      int_frames2drop = metronome_check_sync (frameattr);
      /* check again every SYNC_CHECK_NTH_FRAME frame (vsync_cfg.h) */
      int_frames_until_check = VSYNC_CHECK_NTH_FRAME;
    }				/* end we should check sync  */

  /* done */
  return int_frames2drop;
}


/*
 * unregister from metronome
 */
void
metronome_close (control_struct_t * control)
{
  /* audio thread */
  if (control->streaminfo.type == STREAM_TYPE_AUDIO)
  {
    /* wait if sombody just uses audio pt */
    l4semaphore_down (&sem_audio);

    /* indicate we've got an audio worker */
    flag_audio_as_time_src = 0;

    /* remember for calling ao_getPosition */
    audio_attr = NULL;
    ao_getPosition = NULL;

    /* end wait if sombody just uses audio pt */
    l4semaphore_up (&sem_audio);
  }
  dbl_msec_current_time = 0;
}

/*
 * check's if A/V is synchron. if not it return noof frames to drop
 * or wait's a few millisec to resync
 */

inline static int
metronome_check_sync (frame_ctrl_t * frameattr)
{
  /* no. of frames to drop */
  int int_frames2drop = 0;
  /* the presentation time stamp (pts) in msec of the current video frame */
  double dbl_msec_pts;
  /* A/V-difference <0 too slow, >0 too fast in msec */
  double dbl_msec_av_diff;

  /* just to ensure we're not waiting */
  sl_usec_av_async = 0;

  /* if pts wasn't calculated by core or demuxer, we do it here */
  if (!frameattr->pts)
    dbl_msec_pts =
      (double) (((frameattr->frameID) * 1000) / frameattr->vi.framerate);
  else
    dbl_msec_pts = frameattr->pts;


  /* audio + video ? use soundcard timer as reference */
  if (flag_audio_as_time_src)
  {
    /* wait if sombody just uses audio pt */
    l4semaphore_down (&sem_audio);

    ao_getPosition (audio_attr, &dbl_msec_current_time);
    /* compensate clockdrift between system time and audio card */
    if (flag_dont_calculate_clockdrift)
      metronome_calculate_soundcard_system_clockdrift ((double) get_time_microsec (), 
	                                               dbl_msec_current_time * 1000, /* msec -> 탎ec */
						       frameattr->vi.framerate);

    /* end wait if sombody just uses audio pt */
    l4semaphore_up (&sem_audio);

    /* also adjust video frame pts by clock_factor */
    dbl_msec_pts = dbl_msec_pts / dbl_clock_drift_factor;

  }
  /* video only ? use system timer as reference, don't forget to adjust by dbl_last_video_sync_pts */
  else
  {
    dbl_msec_current_time =
      (double) (get_time_millisec () - ul_msec_video_start_time);
    dbl_msec_current_time += dbl_last_video_sync_pts;
  }

  /* now calculate difference between video_pts and audio/system timer */
  dbl_msec_av_diff = dbl_msec_pts - dbl_msec_current_time;

  LOGdL (DEBUG_SYNC_HEAVY, "frameID=%i, framerate=%i, cur=%i, pts=%i [msec]",
	  (int) frameattr->frameID, (int) frameattr->vi.framerate,
	  (int) dbl_msec_current_time, (int) dbl_msec_pts);

  /* lip-synchron? if(abs(diff(audio - video))>= 80.00) then inform user */
  if ((dbl_msec_av_diff >= 80.00) || (dbl_msec_av_diff <= -80.00))
  {
    LOG ("!sync [%i msec] (too %s)", (int) dbl_msec_av_diff,
	(dbl_msec_av_diff < 0) ? "slow" : "fast");
  }				/* end abs(diff)>80) */

  /* compare pts and current time */
  if ((int) (dbl_msec_av_diff) > 0)
    /* too fast ? wait a few microsecs */
  {
    /* but only sleep 1sec to be responsive */
    if (dbl_msec_av_diff > 1000)
    {
      dbl_msec_av_diff = 1000;
      /* but force new check after waiting */
      int_frames_until_check = 0;
    }
    sl_usec_av_async = (signed long) (dbl_msec_av_diff * (-1000));	/* msec -> 탎ec */
#if DEBUG_SYNC_HEAVY // use printf to have short lines
    printf ("!slowdown/sleep %ldmsec\n", -sl_usec_av_async / 1000);
#endif
    int_frames2drop = 0;
  }
  /* too slow ? drop frames or speedup ? */
  else
  {
    int_frames2drop =
      (int) (double) (((-dbl_msec_av_diff) * frameattr->vi.framerate) / 1000);
    /* speedup */
    if (int_frames2drop <= VSYNC_HARD_DROP_FRAMES)
    {
      /* remember the A/V-async in 탎ec */
      sl_usec_av_async = (signed long) (dbl_msec_av_diff * (-1000));	/* msec -> 탎ec */
#if DEBUG_SYNC_HEAVY // use printf to have short lines
      printf ("!speedup %ldmsec\n", sl_usec_av_async / 1000);
#endif
      int_frames2drop = 0;
    }
    /* hardrop frames */
    else
    {
      /* harddrop max 25 frames */
      if (int_frames2drop > 25)
      {
	int_frames2drop = 25;
	/* force recheck after */
	if (int_frames_until_check > 25)
	  int_frames_until_check = 25;
      }
      LOGdL (DEBUG_SYNC_HEAVY, "!harddrop %d frames", int_frames2drop);
    }
  }

  /* return noof frames to drop */
  return int_frames2drop;
}


/*
 * try to determine the pc and soundcard clock drift and adjust metronome 
 */
/* remeber last clockfactor */
static unsigned long ul_usec_last_frame_period = 10;
/* count how many times it's unchanged. if this reaches 5, we have an stable clockfactor 
   and don't calculate anymore */
static int stable_count = 0;

inline static void
metronome_calculate_soundcard_system_clockdrift (double dbl_usec_system_time,
						 double
						 dbl_usec_audio_position,
						 double dbl_framerate)
{
  double clock_factor;

  /* we have an stable clockfactor  and don't calculate anymore */
  if (stable_count >= 5)
  {
    /* pleases metronome not to call this function */
    LOGdL (DEBUG_SYNC, "Declare clock drift as stable.");
    flag_dont_calculate_clockdrift = 1;
    return;
  }

  /*
   * the time between two calls of this function, then
   * calculate difference bettween pc-time and audio-time (each for one loop) 
   * negative value means video faster than audio 
   */
  clock_factor = dbl_usec_system_time - dbl_usec_last_system_time;
  clock_factor /= (dbl_usec_audio_position - dbl_usec_last_audio_position);

  //cr7_printf("sync.c: clockfact=%2.3f\n",clock_factor);

  /* ignore first run after init or resync in sync calculation 
     also ignore too high (wrong?) values (5 percent should be enough, but vmware is sometimes about 25%) 
   */
  if ((dbl_usec_last_system_time != 0) && (dbl_usec_last_audio_position != 0)
      && (clock_factor > 0.85) && (clock_factor < 1.15))
  {
    /* calculate average clock drift with a sliding mean algorithm */
    dbl_clock_drift_factor *= 0.95;
    dbl_clock_drift_factor += 0.05 * clock_factor;

    ul_usec_frame_period =
      (unsigned long) (double) (1000000 / dbl_framerate) *
      dbl_clock_drift_factor;
    /* remeber last frameperiod  
     * count how many times it's unchanged. if this reaches 5, we have an stable clockfactor 
     * and don't calculate anymore */
    if (ul_usec_last_frame_period == ul_usec_frame_period)
      stable_count++;
    else
      stable_count = 0;
    ul_usec_last_frame_period = ul_usec_frame_period;

  }
  else
    LOGdL (DEBUG_SYNC, "clock factor to small/big - ignored.\n");

  /* save old values for next cycle */
  dbl_usec_last_system_time = dbl_usec_system_time;
  dbl_usec_last_audio_position = dbl_usec_audio_position;
}


double
metronome_get_position (void)
{
  double dbl_msec_return_time = dbl_msec_current_time;

  /* return current time if we use it, if not we check if we've got audio */
  if (dbl_msec_return_time == 0)
  {
    /* do we have audio? may be audio only */
    l4semaphore_down (&sem_audio);
    if ((flag_audio_as_time_src) && (ao_getPosition != NULL))
    {
      ao_getPosition (audio_attr, &dbl_msec_return_time);
    }
    l4semaphore_up (&sem_audio);
  }
  /* return time in msec */
  return dbl_msec_return_time;
}
