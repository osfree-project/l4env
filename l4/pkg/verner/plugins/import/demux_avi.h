/*
 * \brief   AVI container plugin for VERNER
 * \date    2004-09-09
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

#ifndef _DEMUX_AVI_H
#define _DEMUX_AVI_H

#include "arch_globals.h"

/* video functions */
int vid_import_avi_init (plugin_ctrl_t * attr, stream_info_t * info);
int vid_import_avi_commit (plugin_ctrl_t * attr);
int vid_import_avi_step (plugin_ctrl_t * attr, void *addr);
int vid_import_avi_close (plugin_ctrl_t * attr);
int vid_import_avi_seek (plugin_ctrl_t * attr, void *addr, double position,
			 int whence);

/* audio functions */
int aud_import_avi_init (plugin_ctrl_t * attr, stream_info_t * info);
int aud_import_avi_commit (plugin_ctrl_t * attr);
int aud_import_avi_step (plugin_ctrl_t * attr, void *addr);
int aud_import_avi_close (plugin_ctrl_t * attr);
int aud_import_avi_seek (plugin_ctrl_t * attr, void *addr, double position,
			 int whence);


#endif
