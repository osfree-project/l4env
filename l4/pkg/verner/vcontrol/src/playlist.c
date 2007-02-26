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


/* dope */
#include <dopelib.h>

/* l4 */
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/util/rand.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dsi/dsi.h>
#include <l4/semaphore/semaphore.h>

/* env */
#include <stdlib.h>
#include <stdarg.h>

/* local includes */
#include "arch_globals.h"
#include "arch_plugins.h"
#include <l4/vsync/functions.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include "player-UI.h"
#include "mutex.h"

/* verner components */
#include <l4/vcore/functions_video.h>
#include <l4/vcore/functions_audio.h>
#include <l4/vdemuxer/functions.h>
#include <l4/vsync/functions.h>

/* configuration */
#include "verner_config.h"

/* mutliple instance support */
#include "minstances.h"
extern int instance_no;

/* local */
#include "helper.h"
extern connect_chain_t video_chain;
extern connect_chain_t audio_chain;
#include "playlist.h"
#include "player-UI.h"

/* state vars - must be accessable by other c-files */
int pl_entries = 0;		/* noof entries in playlist */
pl_entry_t pl_first_elem = { 0, 1, NULL, NULL, NULL };	/* first element */
pl_entry_t *pl_last_elem = &pl_first_elem;	/* last element */



/*
 * add file to playlist 
 */
void
playlist_add (char *filename, char *info, double length)
{
  pl_entry_t *new_entry = (pl_entry_t *) malloc (sizeof (pl_entry_t));
  pl_entry_t *after_entry = NULL;
  pl_entry_t *before_entry = &pl_first_elem;
  /* find first free id */
  new_entry->id = playlist_find_first_free_id ();
  /* find first free index */
  new_entry->row = playlist_find_first_free_row ();
  /* find elements to add entry after and before */
  while (before_entry)
  {
    if ((new_entry->id - before_entry->id) == 1)
    {
      after_entry = before_entry->next;
      break;
    }
    before_entry = before_entry->next;
  }
  if (!before_entry)
    Panic ("Invalid playlist! Report as BUG");
  /* add to in-mem list */
  new_entry->filename = strdup (filename);
  before_entry->next = new_entry;
  new_entry->prev = before_entry;
  if (after_entry)
  {
    /* add in middle  */
    after_entry->prev = new_entry;
    new_entry->next = after_entry;
  }
  else
  {
    /* add at end of list  */
    pl_last_elem = new_entry;
    new_entry->next = NULL;
  }

  /* now add to playlist window */
  /* button w/ name */
  vdope_cmdf (app_id, "playlist_btn_%i=new Button()", new_entry->id);
  vdope_cmdf (app_id, "playlist_btn_%i.set(-text \"%s\")", new_entry->id,
	      info ? info : filename);
  vdope_cmdf (app_id,
	      "playlist_grid.place(playlist_btn_%i, -column 1 -row %i)",
	      new_entry->id, new_entry->row);
  vdope_bindf (app_id, "playlist_btn_%i", "press", &playlist_play_callback,
	       (void *) new_entry->id, new_entry->id);
  /* label for length */
  vdope_cmdf (app_id, "playlist_label_%i=new Label()", new_entry->id);
  if (length != 0.00)
  {
    vdope_cmdf (app_id, "playlist_label_%i.set(-text \"%.2d:%.2d\")",
		new_entry->id, (int) length / 60, (int) length % 60);
  }
  else
  {
    vdope_cmdf (app_id, "playlist_label_%i.set(-text \"\")", new_entry->id);
  }
  vdope_cmdf (app_id,
	      "playlist_grid.place(playlist_label_%i, -column 2 -row %i)",
	      new_entry->id, new_entry->row);
  /* remove button */
  vdope_cmdf (app_id, "playlist_btn_rm%i=new Button()", new_entry->id);
  vdope_cmdf (app_id, "playlist_btn_rm%i.set(-text \"x\")", new_entry->id);
  vdope_cmdf (app_id,
	      "playlist_grid.place(playlist_btn_rm%i, -column 3 -row %i)",
	      new_entry->id, new_entry->row);
  vdope_bindf (app_id, "playlist_btn_rm%i", "commit",
	       &playlist_change_callback, (void *) new_entry->id,
	       new_entry->id);
  /* adjust row size */
  vdope_cmdf (app_id, "playlist_grid.rowconfig(%i, -size 15)",
	      new_entry->row);
  /* count down files in playlist */
  pl_entries++;
}



/*
 * remove file from playlist 
 */
void
playlist_remove (int id)
{
  pl_entry_t *entry = pl_first_elem.next;	/* the first one can't be removed */
  pl_entry_t *tmp_entry;
  while (entry)
  {
    if (entry->id == id)
    {
      /* remove from playlist */
      vdope_cmdf (app_id, "playlist_grid.rowconfig(%i, -size 0)", entry->row);
      vdope_cmdf (app_id, "playlist_grid.remove(playlist_btn_rm%i)", id);
      vdope_cmdf (app_id, "playlist_grid.remove(playlist_btn_%i)", id);
      vdope_cmdf (app_id, "playlist_grid.remove(playlist_label_%i)", id);
      /* found - set pointers to prev and next elem */
      if ((tmp_entry = (pl_entry_t *) (entry->next)) != NULL)
	tmp_entry->prev = entry->prev;
      else
	pl_last_elem = entry->prev;
      if ((tmp_entry = (pl_entry_t *) (entry->prev)) != NULL)
	tmp_entry->next = entry->next;
      /* free data and elem */
      if (entry->filename)
	free (entry->filename);
      free (entry);
      /* count down files in playlist */
      pl_entries--;
      break;
    }
    entry = entry->next;
  }
}


/*
 * clear playlist 
 */
void
playlist_clear ()
{
  /* runing through the playlist and kill all entries */
  pl_entry_t *entry = pl_last_elem;
  while (entry->id)
  {
    playlist_remove (entry->id);
    entry = entry->prev;
  }
}


/*
 * find an entry by id 
 */
pl_entry_t *
playlist_get_entry_by_id (int id)
{
  pl_entry_t *entry = pl_first_elem.next;
  pl_entry_t *nearest = &pl_first_elem;
  int diff = 9999999;		/* difference between searched id and nearest found so far */

  while (entry)
  {
    if (entry->id == id)
    {
      /* found */
      return entry;
    }
    else
      /* remember nearest found */
    if (abs (entry->id - id) < diff)
    {
      nearest = entry;
      diff = abs (entry->id - id);
    }
    entry = entry->next;
  }
  /* not found - so take "nearest to index" */
  return nearest;
}


/*
 * searches first free id 
 */
int
playlist_find_first_free_id ()
{
  pl_entry_t *entry = &pl_first_elem;
  pl_entry_t *tmp_entry;
  int free_id = 0;
  int last_id = 0;
  /* runing through the playlist and find first free id */
  while (entry)
  {
    tmp_entry = (pl_entry_t *) entry->next;
    /* not valid free id? */
    if (entry->id == free_id)
      free_id = 0;
    /* remember highest id no. found */
    if (entry->id >= last_id)
      last_id = entry->id;
    /* are we at end of list? */
    if (!tmp_entry)
    {
      if (free_id > 0)
	return free_id;
      else
	return last_id + 1;
    }
    /* check if there's cap between current and next */
    if ((tmp_entry->id - entry->id) > 1)
      free_id = entry->id + 1;
    /* process next entry */
    entry = entry->next;
  }
  Panic ("Invalid playlist! Report as BUG");
  return 0;
}

/*
 * searches first free row in playlist window
 */
int
playlist_find_first_free_row ()
{
  /* currently is row=id+1 */
  return (playlist_find_first_free_id () + 1);
}


/*
 * find an entry by filename 
 */
pl_entry_t *
playlist_get_entry_by_filename (char *filename)
{
  pl_entry_t *entry = pl_first_elem.next;

  while (entry)
  {
    if (!strcmp (entry->filename, filename))
    {
      /* found */
      return entry;
    }
    entry = entry->next;
  }
  /* not found  */
  return NULL;
}

/*
 * highlights file currently playing
 */
void
playlist_gui_highlight (int id)
{
  static int last_highlighted_id = -1;	/* currently no one is highlighted */

  /* not in text only mode */
  if (gui_state.text_only)
    return;

  /* unstate last entry if valid */
  if ((last_highlighted_id != -1)
      && (playlist_get_entry_by_id (last_highlighted_id)))
    vdope_cmdf (app_id, "playlist_btn_%i.set(-state 0)", last_highlighted_id);

  /* state next entry if valid */
  if ((id != -1) && (playlist_get_entry_by_id (id)))
    vdope_cmdf (app_id, "playlist_btn_%i.set(-state 1)", id);

  /* remember next entry */
  last_highlighted_id = id;
}



/*
 * updates playlist entries in window
 */
void
playlist_gui_update (int id, char *info, double length)
{

  /* not in text only mode */
  if (gui_state.text_only)
    return;

  /* update info */
  if ((info) && (strlen (info) > 0))
    vdope_cmdf (app_id, "playlist_btn_%i.set(-text \"%s\")", id, info);
  /* label for length */
  if (length != 0.00)
    vdope_cmdf (app_id, "playlist_label_%i.set(-text \"%.2d:%.2d\")", id,
		(int) length / 60, (int) length % 60);

}


/*
 * callback to toggle playlist visible
 */
void
playlist_show_callback (dope_event * e, void *arg)
{
  /* is playlist window visible ? */
  static int visible = 0;
  if (!visible)
  {
    /* show */
    vdope_cmdf (app_id, "main_btn_pl.set(-state 1)");
    vdope_cmd (app_id, "playlist_win.open()");
    vdope_cmd (app_id, "playlist_win.top()");
    visible = 1;
  }
  else
  {
    /* hide */
    vdope_cmdf (app_id, "main_btn_pl.set(-state 0)");
    vdope_cmd (app_id, "playlist_win.close()");
    visible = 0;
  }
}



/*
 * callback to add or remove items from list
 */
void
playlist_change_callback (dope_event * e, void *arg)
{
  char result[256];		/* vdope_req result */
  int cmd = (int) arg;

  if (cmd == CMD_ADD)
  {
    /* add file to dialog - get text to add */
    vdope_req (app_id, result, 256, "playlist_entry.text");
    if (strlen (result) > 0)
    {
      /* clr text */
      vdope_cmd (app_id, "playlist_entry.set(-text \"\")");
      /* add to list */
      event_manager (EV_PLAYLIST_ADD, result, 0);
    }
  }
  else
    /* delete file from list */
    event_manager (EV_PLAYLIST_REM, NULL, cmd);
}



/*
 * callback to play file  
 */
void
playlist_play_callback (dope_event * e, void *arg)
{
  int id = (int) arg;
  pl_entry_t *entry = playlist_get_entry_by_id (id);
  if ((!entry) || (!entry->filename))
    setInfo ("stream not in playlist");
  else
    /* now open and play file */
    event_manager (EV_PLAY, entry->filename, 0);
}
