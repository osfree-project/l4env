/*
 * \brief   Clientlib for VERNER's audio core component
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

#ifndef _VCORE_AUDIO_FUNCTIONS_H
#define _VCORE_AUDIO_FUNCTIONS_H

/* server name as registered by nameserver */
#define VCORE_AUDIO_NAME "VCoreAudio"

#include <l4/dsi/types.h>

/************************/
/*  comquad interface   */
/************************/

/* (dis)connect video transfer - outgoing */
int VideoCoreComponentIntern_connect_UncompressedAudioOut (l4_threadid_t
							   thread_id,
							   const
							   l4dm_dataspace_t *
							   ctrl_ds,
							   const
							   l4dm_dataspace_t *
							   data_ds,
							   dsi_component_t *
							   component);
int VideoCoreComponentIntern_disconnect_UncompressedAudioOut (l4_threadid_t
							      thread_id,
							      const l4_int32_t
							      close_socket_flag);


/* (dis)connect video transfer - incoming */
int VideoCoreComponentIntern_connect_CompressedAudioIn (l4_threadid_t
							thread_id,
							const l4dm_dataspace_t
							* ctrl_ds,
							const l4dm_dataspace_t
							* data_ds,
							dsi_component_t *
							component);
int VideoCoreComponentIntern_disconnect_CompressedAudioIn (l4_threadid_t
							   thread_id,
							   const l4_int32_t
							   close_socket_flag);

/************************/
/* functional interface */
/************************/
l4_int32_t
VideoCoreComponentIntern_setAudioRTparams (l4_threadid_t thread_id,
					   unsigned long period,
					   unsigned long reservation_audio,
					   unsigned long reservation_video,
					   int verbose_preemption_ipc);

EXTERN_C_END
#endif
