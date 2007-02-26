/*
 * \brief   Audio specific for VERNER's sync component
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

#ifndef _AO_OSS_H_
#define _AO_OSS_H_

#include "arch_globals.h"

int ao_oss_init (plugin_ctrl_t * attr, stream_info_t * info);
int ao_oss_commit (plugin_ctrl_t * attr);
inline int ao_oss_step (plugin_ctrl_t * attr, void *addr);
int ao_oss_close (plugin_ctrl_t * attr);
int ao_oss_setVolume (plugin_ctrl_t * attr, int left, int right);
int ao_oss_getPosition (plugin_ctrl_t * attr, double *position);

#endif
