/*
 * \brief   IPC and event server for VERNER's sync component
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

#ifndef _DICE_SERVER_H_
#define _DICE_SERVER_H_

int VideoSyncComponent_dice_thread (void);

/*
 * events handled by request server 
 */
#define SYNC_EVENT_NULL		0
#define SYNC_EVENT_STOP		10
#define SYNC_EVENT_PLAY		11
#define SYNC_EVENT_PAUSE	12
#define SYNC_EVENT_DROP		13
/* more to come */
int VideoSyncComponent_event (int event, char *osd_text);

#endif
