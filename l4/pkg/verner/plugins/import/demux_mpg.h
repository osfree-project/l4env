/*
 * \brief   MPEG plugin for VERNER (currently it's like RAW)
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

#ifndef _DEMUX_MPG_H_
#define _DEMUX_MPG_H_


#include "arch_globals.h"
#include <l4/env/errno.h>

/* video functions */
int vid_import_mpg_init (plugin_ctrl_t * attr, stream_info_t * info);
int vid_import_mpg_commit (plugin_ctrl_t * attr);
int vid_import_mpg_step (plugin_ctrl_t * attr, void *addr);
int vid_import_mpg_close (plugin_ctrl_t * attr);
int vid_import_mpg_seek (plugin_ctrl_t * attr, void *addr, double position,
			 int whence);

/* audio functions - not implemented yet*/
int aud_import_mpg_init (plugin_ctrl_t * attr, stream_info_t * info);
int aud_import_mpg_commit (plugin_ctrl_t * attr);
int aud_import_mpg_step (plugin_ctrl_t * attr, void *addr);
int aud_import_mpg_close (plugin_ctrl_t * attr);
int aud_import_mpg_seek (plugin_ctrl_t * attr, void *addr, double position,
			 int whence);

#endif
