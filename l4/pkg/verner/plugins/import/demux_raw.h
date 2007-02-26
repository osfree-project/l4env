/*
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _DEMUX_RAW_H_
#define _DEMUX_RAW_H_

#include "arch_globals.h"

/* audio functions */
int vid_import_raw_init (plugin_ctrl_t * attr, stream_info_t * info);
int vid_import_raw_commit (plugin_ctrl_t * attr);
int vid_import_raw_step (plugin_ctrl_t * attr, void *addr);
int vid_import_raw_close (plugin_ctrl_t * attr);
int vid_import_raw_seek (plugin_ctrl_t * attr, void *addr, double position,
			 int whence);

#endif
