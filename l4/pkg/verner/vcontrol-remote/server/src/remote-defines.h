#ifndef REMOTE_DEFINES_H
#define REMOTE_DEFINES_H

typedef struct
{
  char *filename;
  unsigned char file_opened;
  unsigned char file_seekable;
  unsigned int playmode;
  unsigned char playing;
  double position;
  double length;
  unsigned short quality;
  unsigned char QAP;
  unsigned int volume;
  unsigned char mute;

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
} remote_gui_state_t;

extern remote_gui_state_t gui_state;

#endif
