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

#ifndef _POSTPROCENGINE_H
#define _POSTPROCENGINE_H

#include "types.h"

int postProcessEngineCommand (const char *command, const char *ppName,
			      const char *ppOptions);
inline int postProcessEngineStep (control_struct_t * control,
				  unsigned char *buffer);
int postProcessEngineClose (void);

#endif
