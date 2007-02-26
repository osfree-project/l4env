/*
 * \brief   Metronome for VERNER's sync component
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

#ifndef _METRONOME_H_
#define _METRONOME_H_

#include "arch_types.h"
#include "work_loop.h"

void metronome_init (control_struct_t * control);
inline int metronome_wait_sync (frame_ctrl_t * frameattr, control_struct_t * control);
void metronome_close (control_struct_t * control);

double metronome_get_position (void);

void metronome_adjust_video_start_time (unsigned long delay);

#endif
