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

#ifndef _AO_NULL_H_
#define _AO_NULL_H_

#include "arch_globals.h"

/* we use it as an OSS-dummy ! */
#define ao_null_init ao_oss_init
#define ao_null_commit ao_oss_commit
#define ao_null_step   ao_oss_step
#define ao_null_close  ao_oss_close
#define ao_null_getPosition ao_oss_getPosition
#define ao_null_setVolume ao_oss_setVolume

int ao_null_init (plugin_ctrl_t * attr, stream_info_t * info);
int ao_null_commit (plugin_ctrl_t * attr);
inline int ao_null_step (plugin_ctrl_t * attr, void *addr);
int ao_null_close (plugin_ctrl_t * attr);
int ao_null_setVolume (plugin_ctrl_t * attr, int left, int right);
int ao_null_getPosition (plugin_ctrl_t * attr, double *position);

#endif
