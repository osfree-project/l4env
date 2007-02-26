/*
 * \brief   Player User interface for  VERNER's control component
 * \date    2004-05-14
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

#ifndef __PLAYLIST_H_
#define __PLAYLIST_H_

/* local */
#include "defines.h"

/*
 * add file to playlist 
 */
void playlist_add (char *filename, char *info, double length);
/*
 * remove file from playlist 
 */
void playlist_remove (int id);
/*
 * clear playlist 
 */
void playlist_clear (void);
/*
 * find an entry by id 
 */
pl_entry_t *playlist_get_entry_by_id (int id);
/*
 * find an entry by filename 
 */
pl_entry_t *playlist_get_entry_by_filename (char *filename);
/*
 * searches first free id 
 */
int playlist_find_first_free_id (void);
/*
 * searches first free row in playlist window
 */
int playlist_find_first_free_row (void);
/*
 * highlights file currently playing
 */
void playlist_gui_highlight (int id);
/*
 * updates playlist entries in window
 */
void playlist_gui_update (int id, char *info, double length);

/*
 * callback to toggle playlist visible
 */
void playlist_show_callback (dope_event * e, void *arg);
/*
 * callback to play file  
 */
void playlist_play_callback (dope_event * e, void *arg);
/*
 * callback to add or remove items from list
 */
void playlist_change_callback (dope_event * e, void *arg);

#endif
