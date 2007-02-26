/*
 * \brief   Postprocessing filter setup for VERNER's core
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include "postproc.h"
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* execute command */
int
postProcessEngineCommand (const char *command, const char *ppName,
			  const char *ppOptions)
{
  LOG_Error ("audio postprocessing is currently unsupported.");
  return -L4_ENOTSUPP;
}

/* execute selected filters */
inline int
postProcessEngineStep (control_struct_t * control, unsigned char *buffer)
{
  return 0;
}

/* close all filters */
int
postProcessEngineClose ()
{
  return 0;
}
