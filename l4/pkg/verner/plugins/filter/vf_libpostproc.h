/*
 * \brief   Postprocessing filter setup for VERNER's core
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

#ifndef _LIBPOSTPROCFILTER_H
#define _LIBPOSTPROCFILTER_H

#include "arch_globals.h"

int vf_libpostproc_init (plugin_ctrl_t * attr, stream_info_t * info,
			 char *options);
int vf_libpostproc_step (plugin_ctrl_t * attr, unsigned char *buffer);
int vf_libpostproc_close (plugin_ctrl_t * attr);

#endif
