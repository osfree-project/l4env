/*
 * \brief   container specific for VERNER's demuxer
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

#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#include "types.h"
int determineAndSetupImport (control_struct_t * control);

l4_int32_t
containerProbeVideoFile (const char *filename,
			 l4_int32_t vid_trackNo,
			 l4_int32_t aud_trackNo,
			 l4_int32_t * vid_tracks,
			 l4_int32_t * aud_tracks, frame_ctrl_t * streaminfo);

#endif
