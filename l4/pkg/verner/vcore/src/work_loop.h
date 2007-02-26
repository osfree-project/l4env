/*
 * \brief   Work thread w/ workloop for VERNER's core component
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

#ifndef _WORK_LOOP_H_
#define _WORK_LOOP_H_

#include "types.h"

int work_loop_create (control_struct_t * control);

int work_loop_start_receiver (control_struct_t * control);
int work_loop_start_sender (control_struct_t * control);

int work_loop_stop (control_struct_t * control);

/*
 * lock to ensure we don't create two work_threads or sending signals while
 * creating. 
 */
void work_loop_lock (control_struct_t * control);
/* and of course unlock again. */
void work_loop_unlock (control_struct_t * control);

#endif
