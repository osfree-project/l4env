/*
 * \brief   Clientlib for VERNER's sync component
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

#ifndef _VSYNC_FUNCTIONS_H
#define _VSYNC_FUNCTIONS_H

/* server name as registered by nameserver */
#define VSYNC_NAME "VSync"

#include <l4/dsi/types.h>

/************************/
/*  comquad interface   */
/************************/

/* (dis)connect video transfer */
int VideoSyncComponentIntern_connect_UncompressedVideoIn (l4_threadid_t
							  thread_id,
							  const
							  l4dm_dataspace_t *
							  ctrl_ds,
							  const
							  l4dm_dataspace_t *
							  data_ds,
							  dsi_component_t *
							  component);
int VideoSyncComponentIntern_disconnect_UncompressedVideoIn (l4_threadid_t
							     thread_id,
							     const l4_int32_t
							     close_socket_flag);

/* (dis)connect audio transfer */
int VideoSyncComponentIntern_connect_UncompressedAudioIn (l4_threadid_t
							  thread_id,
							  const
							  l4dm_dataspace_t *
							  ctrl_ds,
							  const
							  l4dm_dataspace_t *
							  data_ds,
							  dsi_component_t *
							  component);
int VideoSyncComponentIntern_disconnect_UncompressedAudioIn (l4_threadid_t
							     thread_id,
							     const l4_int32_t
							     close_socket_flag);


/************************/
/* functional interface */
/************************/
l4_int32_t
VideoSyncComponentIntern_setRTparams (l4_threadid_t thread_id,
				      unsigned long period,
				      unsigned long reservation_audio,
				      unsigned long reservation_video,
				      int verbose_preemption_ipc);
l4_int32_t
VideoSyncComponentIntern_getPosition (l4_threadid_t thread_id,
				      double *position);

l4_int32_t
VideoSyncComponentIntern_setVolume (l4_threadid_t thread_id, int left,
				    int right);

l4_int32_t
VideoSyncComponentIntern_setPlaybackMode (l4_threadid_t thread_id, int mode);

l4_int32_t
VideoSyncComponentIntern_setFxPlugin (l4_threadid_t thread_id,
				      int fx_plugin_id);


EXTERN_C_END
#endif
