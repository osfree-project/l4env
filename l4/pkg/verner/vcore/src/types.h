/*
 * \brief   Type definitions for VERNER's core components
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

#ifndef _CORE_TYPES_H_
#define _CORE_TYPES_H_

#include <l4/sys/types.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

#if PREDICT_DECODING_TIME
#  include "predict.h"
#endif

#if VCORE_VIDEO_ENABLE_QAP
/* QAP settings */
typedef struct
{
  /* QAP enabled >0 or manual mode <=0 */
  int useQAP;
  /*
   * use libpostprocess's default filters as QAP levels, if codecs doesn't support
   * it's own levels (decoding only!)
   */
  int usePPasQAP;
  /* these values are already in plugin_ctrl_t :) 
     currently selected level 
     int currentQLevel; 
     highest supported quality level (0...MAX) 
     int maxQLevel; 
   */
} qap_struct_t;
#endif

/* how many packets we can hold at once (sender only)  ! */
#if BUILD_RT_SUPPORT
/* in RT-Mode maximum is PACKETS_IN_BUFFER */
#define MAX_SENDER_PACKETS PACKETS_IN_BUFFER
#else
/* in non-RT-Mode maximum is 1 frame */
#define MAX_SENDER_PACKETS 1
#endif

/* struct to store all data for one packet */
#define STATE_PACKET_FREE	0	/* free, not used or commited */
#define STATE_PACKET_GOT	1	/* packet_get success */
#define STATE_PACKET_FILLED	2	/* data was coded into packet (means valid data for commit) */
typedef struct
{
  /* the packet descriptor */
  dsi_packet_t *packet;
  /* the status of the packet */
  int status;
  /* the data addr */
  void *addr;

} packet_data_t;

#if PREDICT_DECODING_TIME
/* the context for decoding time prediction */
typedef struct
{
  /* the file to store newly learned data */
  char *learn_file;
  /* the file to load previous data from for prediction */
  char *predict_file;
  /* the predictor and its allocator */
  predictor_t *predictor;
  /* prediction functions */
  predictor_t *(*predict_new)(const char *learn_file, const char *predict_file);
  double (*predict)(predictor_t *predictor, unsigned char **data, unsigned *length);
  void (*predict_learn)(predictor_t *predictor, double time);
  void (*predict_eval)(predictor_t *predictor);
  void (*predict_discontinue)(predictor_t *predictor);
  void (*predict_dispose)(predictor_t *predictor);
  /* cumulative decoding time and overhead */
  l4_uint64_t decoding_time;
  l4_uint64_t prediction_overhead;
} prediction_context_t;
#endif

/* this is the structure of all variables, flag, ... each separate work_thread needs */
typedef struct
{
  /* currently in use ! */
  int recv_in_use;
  int send_in_use;

  /* work thread id to ensure we're start only one worker */
  int work_thread;

  /* the socket to receive from */
  dsi_socket_t *recv_socket;
  /* the socket to send to */
  dsi_socket_t *send_socket;

  /* dataspace: */
  /* addr the mapped data_ds starts */
  void *send_start_addr;
  /* destination for sending */
  void *send_dst_addr;
  /* size of destination for sending */
  l4_size_t send_dst_size;
  /* the packet counter for sending */
  int send_counter;

  /* packets: */
  /* recv packet (only one packet) */
  packet_data_t recv_packet;
  /* 
   * send packet (count is frames/chunks per loop) 
   * Note: more then packets in buffer is useless and will deadlock
   * this is checked in workloop!
   */
  packet_data_t send_packet[MAX_SENDER_PACKETS];

  /* the semaphore to start our work thread triggered by receiver */
  l4semaphore_t start_recv_sem;
  /* the semaphore to start our work thread triggered by sender */
  l4semaphore_t start_send_sem;

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
  int (*codec_init) (plugin_ctrl_t * attr, stream_info_t * info);
  int (*codec_step) (plugin_ctrl_t * attr, unsigned char *in_buffer,
		     unsigned char *out_buffer);
  int (*codec_close) (plugin_ctrl_t * attr);

#if VCORE_VIDEO_ENABLE_QAP
  /* QAP settings */
  qap_struct_t qap_settings;
#endif

#if BUILD_RT_SUPPORT
  /* rt support */
  /* if preempter thread is running */
  l4semaphore_t running_preempt;
  /* thread_id of work thread */
  l4_threadid_t work_thread_id;
  /* next period id */
  l4_threadid_t next_period_id;
  /* the number of preemption IPC occured since last QAP check */
  unsigned int rt_preempt_count;
  /* the lenght of one period in µsec */
  unsigned long rt_period;
  /* reservation time */
  unsigned long rt_reservation;
  /* reservation time for optional part */
  unsigned long rt_reservation_opt;
  /* verbose preemption-ipc */
  int rt_verbose_pipc;
#endif

#if PREDICT_DECODING_TIME
  prediction_context_t *predict;
#endif

} control_struct_t;

#endif
