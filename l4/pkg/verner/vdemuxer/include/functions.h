/*
 * \brief   Clientlib for VERNER's demuxer component
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

#ifndef _VDEMUXER_FUNCTIONS_H
#define _VDEMUXER_FUNCTIONS_H

/* server name as registered by nameserver */
#define VDEMUXER_NAME "VDemuxer"

#include <l4/dsi/types.h>
#include "arch_types.h"

/************************/
/*  comquad interface   */
/************************/
int VideoDemuxerComponentIntern_connect_CompressedVideoOut (l4_threadid_t
							    thread_id,
							    const
							    l4dm_dataspace_t *
							    ctrl_ds,
							    const
							    l4dm_dataspace_t *
							    data_ds,
							    dsi_component_t *
							    component);
int VideoDemuxerComponentIntern_disconnect_CompressedVideoOut (l4_threadid_t
							       thread_id,
							       const
							       l4_int32_t
							       close_socket_flag);


int VideoDemuxerComponentIntern_connect_CompressedAudioOut (l4_threadid_t
							    thread_id,
							    const
							    l4dm_dataspace_t *
							    ctrl_ds,
							    const
							    l4dm_dataspace_t *
							    data_ds,
							    dsi_component_t *
							    component);
int VideoDemuxerComponentIntern_disconnect_CompressedAudioOut (l4_threadid_t
							       thread_id,
							       const
							       l4_int32_t
							       close_socket_flag);


/************************/
/* functional interface */
/************************/
l4_int32_t
VideoDemuxerComponentIntern_setRTparams (l4_threadid_t thread_id,
					 unsigned long period,
					 unsigned long reservation_audio,
					 unsigned long reservation_video,
					 int verbose_preemption_ipc);


l4_int32_t
VideoDemuxerComponentIntern_probeVideoFile (l4_threadid_t thread_id,
					    const char *filename,
					    l4_int32_t videoTrackNo,
					    l4_int32_t audioTrackNo,
					    l4_int32_t * videoTracks,
					    l4_int32_t * audioTracks,
					    frame_ctrl_t * streaminfo);


l4_int32_t
VideoDemuxerComponentIntern_setVideoFileParam (l4_threadid_t thread_id,
					       const char *videoFilename,
					       const char *audioFilename,
					       l4_int32_t videoTrackNo,
					       l4_int32_t audioTracks,
					       const char *videoPlugin,
					       const char *audioPlugin);

l4_int32_t
VideoDemuxerComponentIntern_setSeekPosition (l4_threadid_t thread_id,
					     int seekVideo,
					     int seekAudio,
					     double position, 
					     long fileoffset,
					     int whence);

EXTERN_C_END
#endif
