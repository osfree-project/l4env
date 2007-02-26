/*
 * \brief   Sender functions for VERNER's sync component
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

#ifndef _SEND_H_
#define _SEND_H_

#include "types.h"

int sender_open_socket (control_struct_t * control,
			const l4dm_dataspace_t * ctrl_ds,
			const l4dm_dataspace_t * data_ds,
			dsi_socket_ref_t * socketref);

int sender_start (control_struct_t * control, dsi_socket_ref_t * local,
		  dsi_socket_ref_t * remote);

int sender_close (control_struct_t * control, l4_int32_t close_socket_flag);

int sender_commit (control_struct_t * control, int packet_index);
int sender_step (control_struct_t * control, int packet_index);
void sender_signal_eos (control_struct_t * control);

#endif
