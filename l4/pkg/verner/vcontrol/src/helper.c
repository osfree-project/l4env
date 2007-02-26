/*
 * \brief   Helper functions for VERNER's control component
 * \date    2004-03-17
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


/* l4 */
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* local includes */
#include "helper.h"

/* verner components */
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vsync/functions.h>

/* configuration */
#include "verner_config.h"


/*****************************************************************************
 * Helpers
 *****************************************************************************/
/*****************************************************************************/
/**
 * \brief Allocate dataspace
 * 
 * \param  size dataspace size
 * \retval ds   dataspace id 
 * \return 0 on success, -1 if allocation failed.
 */
/*****************************************************************************/
int
__allocate_ds (l4_size_t size, l4dm_dataspace_t * ds)
{
  int ret;

  /* allocate dataspace */
  ret =
    l4dm_mem_open (L4DM_DEFAULT_DSM, l4_round_page (size), 0, 0, "DSI data",
		   ds);
  if (ret < 0)
  {
    Panic ("dataspace allocation failed: %d", ret);
    return -1;
  }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Free dataspace
 * 
 * \param  ds   dataspace id 
 */
/*****************************************************************************/
int
__free_ds (l4dm_dataspace_t * ds)
{
  int ret;

  /* free dataspace */
  ret = l4dm_close (ds);
  if (ret < 0)
  {
    Panic ("dataspace free failed: %d", ret);
    return -1;
  }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief allocate data_ds and ctrl_ds
 * 
 * \param   packet_size size per packet in bytes
 * \retval  ctrl_ds   allocated ctrl dataspace 
 * \retval  data_ds   allocated data dataspace 
 */
/*****************************************************************************/
int
__allocate_ctrl_ds_data_ds (int packet_size, l4dm_dataspace_t * ctrl_ds,
			    l4dm_dataspace_t * data_ds)
{
  dsi_stream_cfg_t cfg;
  int ret;

  /* allocate data area */
  ret = __allocate_ds (packet_size * PACKETS_IN_BUFFER, data_ds);
  if (ret < 0)
  {
    LOG_Error ("allocation of data area failed!");
    return -1;
  }

  /* allocate ctrl ds */
  cfg.num_packets = PACKETS_IN_BUFFER;
  cfg.max_sg = 2;
  ret = dsi_ds_create_ctrl_dataspace (cfg, ctrl_ds);
  if (ret < 0)
  {
    LOG_Error ("allocation of ctrl area failed!");
    /* free data_ds ! */
    __free_ds (data_ds);
    return -1;
  }
  return 0;
}

/*****************************************************************************/
/**
 * \brief handle stream
 * 
 * \param  sender    sender component
 * \param  receiver  receiver component
 * \param  ctrl_ds   already allocated ctrl dataspace 
 * \param  data_ds   already allocated data dataspace 
 * \param  log_str   char* to include into log output
 *
 * creates the stream, starts the stream, wait's for EOS via dsi_stream_select()
 * and closes the stream
 */
/*****************************************************************************/
int
__handle_stream (dsi_component_t * sender1, dsi_component_t * receiver1,
		 dsi_component_t * sender2, dsi_component_t * receiver2,
		 l4dm_dataspace_t ctrl_ds[2], l4dm_dataspace_t data_ds[2],
		 const char *log_str)
{
  dsi_stream_t *stream[2];
  int ret, i;
  dsi_select_socket_t s_select[4];
  dsi_select_socket_t s_events[4];
  int num_events;

  LOGdL (DEBUG_STREAM, "beginning stream handling %s.", log_str);

  /* create streams */
  ret =
    dsi_stream_create (sender1, receiver1, ctrl_ds[0], data_ds[0],
		       &stream[0]);
  if (ret)
  {
    LOG_Error ("create stream 1 %s failed", log_str);
    return -1;
  }
  ret =
    dsi_stream_create (sender2, receiver2, ctrl_ds[1], data_ds[1],
		       &stream[1]);
  if (ret)
  {
    LOG_Error ("create stream 2 %s failed", log_str);
    return -1;
  }

  /* start transfer */
  LOGdL (DEBUG_STREAM, "start stream %s ...", log_str);
  ret = dsi_stream_start (stream[0]);
  if (ret)
  {
    LOG_Error ("start 1 %s failed", log_str);
    return -1;
  }
  ret = dsi_stream_start (stream[1]);
  if (ret)
  {
    LOG_Error ("start 2 %s failed", log_str);
    return -1;
  }

  /* wait for end of one stream */
  s_select[0].stream = stream[1];
  s_select[0].component = DSI_SEND_COMPONENT;
  s_select[0].events = DSI_EVENT_EOS;
  s_select[1].stream = stream[1];
  s_select[1].component = DSI_RECEIVE_COMPONENT;
  s_select[1].events = DSI_EVENT_EOS;
  s_select[2].stream = stream[0];
  s_select[2].component = DSI_SEND_COMPONENT;
  s_select[2].events = DSI_EVENT_EOS;
  s_select[3].stream = stream[0];
  s_select[3].component = DSI_RECEIVE_COMPONENT;
  s_select[3].events = DSI_EVENT_EOS;
  /* now wait */
  ret = dsi_stream_select (s_select, 4, s_events, &num_events);
  if (ret)
  {
    LOG_Error ("select %s failed (%d)", log_str, ret);
    return -1;
  }
  LOGdL (DEBUG_STREAM, "Got signal for %s.", log_str);
  for (i = 0; i < num_events; i++)
  {
    /* if we got out EOS, we close both(demuxer->core & core->sync) streams! */
    if (s_events[i].events & DSI_EVENT_EOS)
    {
      ret = dsi_stream_close (stream[0]);
      if (ret)
	LOG_Error ("stream %s close failed (%d)", log_str, ret);
      ret = dsi_stream_close (stream[1]);
      if (ret)
	LOG_Error ("stream %s close failed (%d)", log_str, ret);
      LOGdL (DEBUG_STREAM, "EOS, closed stream %s done.", log_str);
    }
  }

  /* done */
  return 0;
}


/*****************************************************************************
 * Connect Vdemuxer and VCore and VSync for video transfer
 *****************************************************************************/
void
build_and_connect_video_chain (connect_chain_t * chain)
{
  l4dm_dataspace_t ctrl_ds[2];
  l4dm_dataspace_t data_ds[2];
  dsi_component_t sender[2];
  dsi_component_t receiver[2];
  int connected[4] = { 0, 0, 0, 0 };

  const char *log_str = "Video: VDemuxer, VCore and VSync";

  /* already connected */
  if (chain->connected)
  {
    l4thread_started (NULL);
    return;
  }
  /* if not - indicated it */
  chain->connected = 1;
  l4thread_started (NULL);

  /* allocate ctrl + data_ds */
  if (__allocate_ctrl_ds_data_ds
      (chain->packet_size, &ctrl_ds[0], &data_ds[0]))
  {
    LOG_Error ("__allocate_ctrl_ds_data_ds 1 failed");
    goto close_video_chain;
  }
  if (__allocate_ctrl_ds_data_ds
      (chain->packet_size, &ctrl_ds[1], &data_ds[1]))
  {
    /* free data already allocated */
    __free_ds (&data_ds[0]);
    __free_ds (&ctrl_ds[0]);
    LOG_Error ("__allocate_ctrl_ds_data_ds 2 failed");
    goto close_video_chain;
  }

  /* connect VDemuxer to VCore with ds[0] */
  connected[0] =
    !VideoDemuxerComponentIntern_connect_CompressedVideoOut
    (chain->vdemuxer_thread_id, &ctrl_ds[0], &data_ds[0], &sender[0]);
  connected[1] =
    !VideoCoreComponentIntern_connect_CompressedVideoIn
    (chain->vcore_thread_id, &ctrl_ds[0], &data_ds[0], &receiver[0]);

  /* connect VCore to VSync with ds[1] */
  connected[2] =
    !VideoCoreComponentIntern_connect_UncompressedVideoOut
    (chain->vcore_thread_id, &ctrl_ds[1], &data_ds[1], &sender[1]);
  connected[3] =
    !VideoSyncComponentIntern_connect_UncompressedVideoIn (chain->
							   vsync_thread_id,
							   &ctrl_ds[1],
							   &data_ds[1],
							   &receiver[1]);

  /* all connected ? */
  if ((!connected[0]) || (!connected[1]) || (!connected[2])
      || (!connected[3]))
  {
    LOG_Error ("not all components connected successfully - closing. ");
    /* close already open components */
    if (connected[0])
      VideoDemuxerComponentIntern_disconnect_CompressedVideoOut
	(chain->vdemuxer_thread_id, 1);
    if (connected[1])
      VideoCoreComponentIntern_disconnect_CompressedVideoIn
	(chain->vcore_thread_id, 1);
    if (connected[2])
      VideoCoreComponentIntern_disconnect_UncompressedVideoOut
	(chain->vcore_thread_id, 1);
    if (connected[3])
      VideoSyncComponentIntern_disconnect_UncompressedVideoIn
	(chain->vsync_thread_id, 1);
  }
  /* if sucessfully connected we handle stream (create, start, select) */
  else
  {
    /* set QAP in core as it's in set in GUI */
    int dummy;
    /* change QAP settings in core */
    VideoCoreComponentIntern_changeQAPSettings (chain->vcore_thread_id,
						chain->QAP_on, -1,
						chain->quality, &dummy,
						&dummy);

#if PREDICT_DECODING_TIME || H264_SLICE_SCHEDULE
    /* set prediction */
    VideoCoreComponentIntern_setPrediction(chain->vcore_thread_id,
					   chain->learn_file,
					   chain->predict_file);
#endif


    if (__handle_stream
	(&sender[0], &receiver[0], &sender[1], &receiver[1], ctrl_ds, data_ds,
	 log_str))
    {
      LOG_Error ("__handle_stream failed. EXPECT MORE ERRORS !");
    }
  }

  /* WE HAVE TO BE SURE, that no one is accessing ctrl_ds or data_ds anymore!
   * So the callback functions of dsi_stream_close MUST stop all work_threads
   * and close the DSI-sockets.
   */
  LOGdL (DEBUG_DS, "free data_ds and ctrl_ds %s.", log_str);
  /* free data area */
  __free_ds (&data_ds[0]);
  __free_ds (&data_ds[1]);
  /* free ctrl area */
  __free_ds (&ctrl_ds[0]);
  __free_ds (&ctrl_ds[1]);

close_video_chain:
  chain->connected = 0;

  return;
}



/*****************************************************************************
 * Connect Vdemuxer and VCore and VSync for audio transfer
 *****************************************************************************/
void
build_and_connect_audio_chain (connect_chain_t * chain)
{
  l4dm_dataspace_t ctrl_ds[2];
  l4dm_dataspace_t data_ds[2];
  dsi_component_t sender[2];
  dsi_component_t receiver[2];
  int connected[4] = { 0, 0, 0, 0 };

  const char *log_str = "Audio: VDemuxer, VCore and VSync";

  /* already connected */
  if (chain->connected)
  {
    l4thread_started (NULL);
    return;
  }
  /* if not - indicated it */
  chain->connected = 1;
  l4thread_started (NULL);

  /* allocate ctrl + data_ds */
  if (__allocate_ctrl_ds_data_ds
      (chain->packet_size, &ctrl_ds[0], &data_ds[0]))
  {
    LOG_Error ("__allocate_ctrl_ds_data_ds 1 failed");
    goto close_audio_chain;
  }
  if (__allocate_ctrl_ds_data_ds
      (chain->packet_size, &ctrl_ds[1], &data_ds[1]))
  {
    /* free data already allocated */
    __free_ds (&data_ds[0]);
    __free_ds (&ctrl_ds[0]);
    LOG_Error ("__allocate_ctrl_ds_data_ds 2 failed");
    goto close_audio_chain;
  }

  /* connect VDemuxer to VCore with ds[0] */
  connected[0] =
    !VideoDemuxerComponentIntern_connect_CompressedAudioOut
    (chain->vdemuxer_thread_id, &ctrl_ds[0], &data_ds[0], &sender[0]);
  connected[1] =
    !VideoCoreComponentIntern_connect_CompressedAudioIn
    (chain->vcore_thread_id, &ctrl_ds[0], &data_ds[0], &receiver[0]);

  /* connect VCore to VSync with ds[1] */
  connected[2] =
    !VideoCoreComponentIntern_connect_UncompressedAudioOut
    (chain->vcore_thread_id, &ctrl_ds[1], &data_ds[1], &sender[1]);
  connected[3] =
    !VideoSyncComponentIntern_connect_UncompressedAudioIn (chain->
							   vsync_thread_id,
							   &ctrl_ds[1],
							   &data_ds[1],
							   &receiver[1]);

  /* all connected ? */
  if ((!connected[0]) || (!connected[1]) || (!connected[2])
      || (!connected[3]))
  {
    LOG_Error ("not all components connected successfully - closing. ");
    /* close already open components */
    if (connected[0])
      VideoDemuxerComponentIntern_disconnect_CompressedAudioOut
	(chain->vdemuxer_thread_id, 1);
    if (connected[1])
      VideoCoreComponentIntern_disconnect_CompressedAudioIn
	(chain->vcore_thread_id, 1);
    if (connected[2])
      VideoCoreComponentIntern_disconnect_UncompressedAudioOut
	(chain->vcore_thread_id, 1);
    if (connected[3])
      VideoSyncComponentIntern_disconnect_UncompressedAudioIn
	(chain->vsync_thread_id, 1);
  }
  /* if sucessfully connected we handle stream (create, start, select) */
  else
  {
#if 0 && PREDICT_DECODING_TIME
    /* TODO: audio decoding time prediction is not implemented */
    /* set prediction */
    VideoCoreComponentIntern_setPrediction(chain->vcore_thread_id,
					   chain->learn_file,
					   chain->predict_file);
#endif


    if (__handle_stream
	(&sender[0], &receiver[0], &sender[1], &receiver[1], ctrl_ds, data_ds,
	 log_str))
    {
      LOG_Error ("__handle_stream failed. EXPECT MORE ERRORS !");
    }
  }

  /* WE HAVE TO BE SURE, that no one is accessing ctrl_ds or data_ds anymore!
   * So the callback functions of dsi_stream_close MUST stop all work_threads
   * and close the DSI-sockets.
   */
  LOGdL (DEBUG_DS, "free data_ds and ctrl_ds %s.", log_str);
  /* free data area */
  __free_ds (&data_ds[0]);
  __free_ds (&data_ds[1]);
  /* free ctrl area */
  __free_ds (&ctrl_ds[0]);
  __free_ds (&ctrl_ds[1]);

close_audio_chain:
  chain->connected = 0;

  return;
}
