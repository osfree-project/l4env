/*
 * \brief   Helper functions for  VERNER's control component
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

#ifndef _CONTROL_HELPER_H_
#define _CONTROL_HELPER_H_

/* l4 */
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/dsi/dsi.h>
#include <l4/dm_mem/dm_mem.h>

/* verner */
#include "verner_config.h"

typedef struct
{
  int packet_size;		/* packet size */
  l4_threadid_t vdemuxer_thread_id;
  l4_threadid_t vcore_thread_id;
  l4_threadid_t vsync_thread_id;
  int QAP_on;			/* QAP enabled */
  int quality;			/* quality level for vcore */
  int connected;		/* indication that audio chain is build and connected */
#if PREDICT_DECODING_TIME
  const char *learn_file;
  const char *predict_file;
#endif
} connect_chain_t;

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
int __allocate_ds (l4_size_t size, l4dm_dataspace_t * ds);

/*****************************************************************************/
/**
 * \brief Free dataspace
 * 
 * \param  ds   dataspace id 
 */
/*****************************************************************************/
int __free_ds (l4dm_dataspace_t * ds);

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
			    l4dm_dataspace_t * data_ds);

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
		 const char *log_str);

/*****************************************************************************
 * Connect Vdemuxer and VCore and VSync for video transfer
 *****************************************************************************/
void build_and_connect_video_chain (connect_chain_t * chain);

/*****************************************************************************
 * Connect Vdemuxer and VCore and VSync for audio transfer
 *****************************************************************************/
void build_and_connect_audio_chain (connect_chain_t * chain);


#endif
