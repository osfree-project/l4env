/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#ifndef VERNER
#include <sys/time.h>
#else
#include "timer.h"
#endif

#include "process.h"


#if SLICE_SKIPPING

#ifdef VERNER
static float machine_speed = 1.0;
#endif

#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_HISTOGRAM)
int schedule_skip(const AVCodecContext *c, int current_slice)
{
  static int slice_to_replace = -1;
  static int global_slice_count = 0;
  
  if (slice_to_replace < 0) {
    const char *const envvar = getenv("SKIP_SLICE_NUMBER");
    if (!envvar) {
      printf("the slice number to skip is expected in the environment variable SKIP_SLICE_NUMBER\n");
      exit(1);
    }
    sscanf(envvar, "%d", &slice_to_replace);
  }
  
  if (global_slice_count++ == slice_to_replace && proc.frame->replacement) {
    proc.propagation.first_picture = c->frame.current->coded_picture_number;
    return 1;
  }
  
  if (global_slice_count > slice_to_replace && proc.propagation.first_picture < 0) {
    /* error propagation has finished */
#ifdef PROPAGATION_HISTOGRAM
    propagation_finalize();
#endif
    exit(0);
  }
  
  return 0;
}
#endif

#ifdef PROPAGATION_ACCUMULATE
int schedule_skip(const AVCodecContext *c, int current_slice)
{
  /* skip every 10th slice */
  return (proc.frame->replacement && random() % RAND_MAX < RAND_MAX / 10);
}
#endif

#if defined(SCHEDULING_DEPTH) || defined(SCHEDULING_OVERHEAD) || defined(FINAL_SCHEDULING) || defined(VERNER)
int schedule_skip(const AVCodecContext *c, int current_slice)
{
  static double frame_duration = -1.0;
  static double frame_deadline = -1.0;
  int deadlines_missed = 0;
  frame_node_t *frame;
  int slice;
#ifdef SCHEDULING_OVERHEAD
  double schedule_time = 0.0;
#endif
  
  if (!proc.llsp.predict_coeffs || !proc.frame) return 0;
  
#if SCHEDULING_METHOD == NO_SKIP
  if (c->frame.flag_idr) frame_deadline = -1.0;
#endif
  
#ifdef VERNER
  frame_duration = (1.0 / 25.0) * machine_speed;
#else
  if (frame_duration < 0) {
    float framerate;
    const char *const envvar = getenv("FRAMERATE");
    if (!envvar) {
      printf("the scheduler's target framerate is expected in the environment variable FRAMERATE\n");
      exit(1);
    }
    sscanf(envvar, "%f", &framerate);
    frame_duration = 1.0 / framerate;
  }
#endif
  
  if (frame_deadline < 0)
    /* initialize time */
    frame_deadline = get_time();
  
#ifdef SCHEDULING_OVERHEAD
  schedule_time -= get_time();
#endif
  
  for (frame = proc.frame; frame; frame = frame->next)
    for (slice = 0; slice < frame->slice_count; slice++)
      frame->slice[slice].skip = 0;
  
  do {
    frame_node_t *least_useful_frame = NULL;
    int least_useful_slice = 0;
    double least_useful_benefit = HUGE_VAL;
    double budget;
    
    budget = frame_deadline - get_time();
    /* we simulate the maximum output frame queue here */
    if (budget > output_queue * frame_duration) {
      frame_deadline += output_queue * frame_duration - budget;
      budget = output_queue * frame_duration;
    }
    deadlines_missed = 0;
    
    for (frame = proc.frame; frame; frame = frame->next) {
      /* refresh the time budget with one frame worth of time */
      budget += frame_duration;
      
      for (slice = (frame == proc.frame ? current_slice : 0); slice < frame->slice_count; slice++) {
	if (frame->slice[slice].skip) {
	  /* deplete the budget by the estimated replacement time */
	  budget -= frame->slice[slice].replacement_time;
	} else {
	  /* deplete the budget by the estimated decoding time */
	  budget -= frame->slice[slice].decoding_time;
	  /* remember the slice with the least benefit */
	  if (frame->slice[slice].benefit < least_useful_benefit) {
	    least_useful_benefit = frame->slice[slice].benefit;
	    least_useful_frame = frame;
	    least_useful_slice = slice;
	  }
	}
      }
      
      if (budget < 0.0) {
	/* we have overrun our budget, skip the least useful slice */
	if (least_useful_frame) {
	  least_useful_frame->slice[least_useful_slice].skip = 1;
	  deadlines_missed = 1;
	}
	break;
      }
    }
  } while (deadlines_missed && !proc.frame->slice[current_slice].skip);
  

  if (current_slice == proc.frame->slice_count - 1)
    /* last slice, prepare the deadline */
    frame_deadline += frame_duration;
  
#if defined(SCHEDULING_DEPTH)
  if (proc.frame->slice[current_slice].skip) {
    frame_node_t *frame2;
    int i;
    for (i = 0, frame2 = proc.frame; frame2; i++, frame2 = frame2->next)
      if (frame2 == frame) break;
    printf("%d\n", i);
  }
#elif defined(SCHEDULING_OVERHEAD)
  schedule_time += get_time();
  printf("%lf\n", schedule_time);
#elif SCHEDULING_METHOD == NO_SKIP
  printf("%d\n", 3 * ((proc.frame->replacement) && (frame_deadline - get_time() < 0)));
#elif !defined(VERNER)
  printf("%d\n", proc.frame->slice[current_slice].skip);
#endif
  
#if SCHEDULING_METHOD == NO_SKIP
  return 0;
#else
  return proc.frame->slice[current_slice].skip;
#endif
}
#endif

#ifdef SCHEDULE_EXECUTE
int schedule_skip(const AVCodecContext *c, int current_slice)
{
  int conceal;
  
  fscanf(stdin, "%d\n", &conceal);
  switch (conceal) {
  case 2:
    /* use FFmpeg's concealment */
    proc.schedule.conceal = 1;
    proc.schedule.first_to_drop = -1;
    break;
  case 3:
    /* drop the whole frame */
    proc.schedule.conceal = 0;
    if (proc.schedule.first_to_drop < 0)
      proc.schedule.first_to_drop = c->frame.current->coded_picture_number;
    conceal = 0;
    break;
  default:
    proc.schedule.conceal = 0;
    proc.schedule.first_to_drop = -1;
  }
  return conceal;
}
#endif

#endif

#if LLSP_TRAIN || defined(LLSP_PREDICTION) || defined(SCHEDULING_DEPTH) || defined(SCHEDULING_OVERHEAD) || defined(FINAL_SCHEDULING) || defined(VERNER)
double get_time(void)
{
#ifndef VERNER
  struct timeval time;
  gettimeofday(&time, NULL);
  return (double)time.tv_sec + (double)time.tv_usec / 1E6;
#else
  return (double)get_thread_time_microsec() / 1E6;
#endif
}
#endif

#ifdef VERNER
void h264_prediction_files(const char *train, const char *predict)
{
  proc.llsp.train_coeffs   = (train   && train[0])   ? train   : NULL;
  proc.llsp.predict_coeffs = (predict && predict[0]) ? predict : NULL;
}
void h264_machine_speed(float speed)
{
  machine_speed = speed;
}
#endif
