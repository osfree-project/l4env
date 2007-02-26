/*
 * \brief   Effects using GOOM for VERNER's sync
 * \date    2004-05-19
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

#ifndef _FX_GOOM_H
#define _FX_GOOM_H

#include "arch_globals.h"

int fx_goom_init (plugin_ctrl_t * attr, stream_info_t * info, char *options);
int fx_goom_step (plugin_ctrl_t * attr, unsigned char *buffer);
int fx_goom_close (plugin_ctrl_t * attr);

#endif
