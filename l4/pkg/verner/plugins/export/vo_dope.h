/*
 * \brief   Video specific for VERNER's sync component
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

#ifndef _VO_DOPE_H_
#define _VO_DOPE_H_

#include "arch_globals.h"

int vo_dope_init (plugin_ctrl_t * attr, stream_info_t * info);
int vo_dope_commit (plugin_ctrl_t * attr);
int vo_dope_step (plugin_ctrl_t * attr, void *addr);
int vo_dope_close (plugin_ctrl_t * attr);

#endif
