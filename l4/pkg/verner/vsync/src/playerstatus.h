/*
 * \brief   Controls playerstatus for VERNER's sync component
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
#ifndef _PLAYERSTATUS_H_
#define _PLAYERSTATUS_H_


void init_playerstatus (int streamtype);
void close_playerstatus (int streamtype);

/* set player mode */
void set_player_mode (int mode);

/*
 * check player mode:
 * wait's until mode. if mode = 0 it's not waiting
 * streamtype is AUDIO|VIDEO ...
 * return: current mode (after waiting)
 */
inline int check_player_mode (int wait_until_modi, int streamtype);

#endif
