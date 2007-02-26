/*
 * \brief   Video specific for VERNER's sync component
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

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/* libc */
#include <stdlib.h>		/*atoi */

/* DOpE */
#include <dopelib.h>
#include <vscreen.h>
#include "keycodes.h"

/* verner */
#include "arch_globals.h"
#include "request_server.h"	/* event handler of sync component */
#include "verner_config.h"

/* Local includes */
#include "vo_dope.h"
#include "osd.h"
#include "timer.h"

/* define types as DOpE does */
#undef u16
#define u16 unsigned short

/* aclib */
#include "aclib.h"
void *(*fastmemcpy_func) (void *to, const void *from, size_t len);

/* place here the best matches for DOpEs vsreen support */
#define DOPE_FMT 		VID_FMT_RAW
#define DOPE_COLORSPACE 	VID_YUV420
#define DOPE_CP_INIT_STR 	"vernervscr.setmode(%d,%d,\"YUV420\")"

/* attr from/for DOpE */
typedef struct
{
  long app_id;			/* DOpE application id */
  void *vernervscr_id;		/* vscreen id */
  u16 *scr_addr;		/* addr for shared mem */
  int scr_width;		/* screen size */
  int scr_height;
  int xdim;			/* video size */
  int ydim;
  int memcpy_size;		/* size to copy */
  char *osd_text;		/* string to display via OSD */
  int osd_frames_to_display;	/* frames to show osd */
  int fullscreen;
  int pause;
} gui_state_t;
static gui_state_t gui_state;

/* flag if dope is initialized - this should only be done ONCE !! */
static int vo_dope_initialized = 0;


/* 
 * dope event handler thread
*/
static void
dope_event_handler_thread (void)
{
  dope_eventloop (gui_state.app_id);
}

/*
 * callback for events 
 */
static void
vo_dope_event_callback (dope_event * e, void *arg)
{

  if (e->type == EVENT_TYPE_PRESS)
  {
    switch (e->press.code)
    {

    case DOPE_KEY_F:
    case DOPE_BTN_RIGHT:
      /* toggle fullscreen */
      gui_state.fullscreen ^= 1;
      if (gui_state.fullscreen)
      {
	VideoSyncComponent_event (SYNC_EVENT_NULL, "Fullscreen");
	dope_cmdf (gui_state.app_id,
		   "vernerwin.set(-workx 0 -worky 0 -workw %d -workh %d)",
		   gui_state.scr_width, gui_state.scr_height);
      }
      else
      {
	VideoSyncComponent_event (SYNC_EVENT_NULL, "Size 100%");
	dope_cmdf (gui_state.app_id,
		   "vernerwin.set(-workx %d -worky %d -workw %d -workh %d)",
		   (gui_state.scr_width / 2) - (gui_state.xdim / 2),
		   (gui_state.scr_height / 2) - (gui_state.ydim / 2),
		   gui_state.xdim, gui_state.ydim);
      }
      break;
    case DOPE_KEY_0:
      /* size 50% */
      VideoSyncComponent_event (SYNC_EVENT_NULL, "Size 50%");
      dope_cmdf (gui_state.app_id,
		 "vernerwin.set(-workx %d -worky %d -workw %d -workh %d)",
		 (gui_state.scr_width / 2) - (gui_state.xdim / 4),
		 (gui_state.scr_height / 2) - (gui_state.ydim / 4),
		 gui_state.xdim / 2, gui_state.ydim / 2);
      break;

    case DOPE_KEY_1:
      /* size 100% */
      VideoSyncComponent_event (SYNC_EVENT_NULL, "Size 100%");
      dope_cmdf (gui_state.app_id,
		 "vernerwin.set(-workx %d -worky %d -workw %d -workh %d)",
		 (gui_state.scr_width / 2) - (gui_state.xdim / 2),
		 (gui_state.scr_height / 2) - (gui_state.ydim / 2),
		 gui_state.xdim, gui_state.ydim);
      break;

    case DOPE_KEY_2:
      /* size 200% */
      VideoSyncComponent_event (SYNC_EVENT_NULL, "Size 200%");
      dope_cmdf (gui_state.app_id,
		 "vernerwin.set(-workx %d -worky %d -workw %d -workh %d)",
		 (gui_state.scr_width / 2) - (gui_state.xdim),
		 (gui_state.scr_height / 2) - (gui_state.ydim),
		 gui_state.xdim * 2, gui_state.ydim * 2);
      break;

    case DOPE_KEY_P:
    case DOPE_BTN_LEFT:
      /* toggle pause */
      gui_state.pause ^= 1;
      if (gui_state.pause)
	VideoSyncComponent_event (SYNC_EVENT_PAUSE, NULL);
      else
	VideoSyncComponent_event (SYNC_EVENT_PLAY, NULL);
      break;

    case DOPE_KEY_ESC:
      /* stop */
      VideoSyncComponent_event (SYNC_EVENT_STOP, NULL);

    }				/* end switch */

  }				/* end keypressed */
}



/*
 * open & init dope export plugin
 */
int
vo_dope_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  char req_buf[16];		/* receive buffer for dope_req */

  if ((info->vi.xdim == 0) || (info->vi.ydim == 0))
  {
    LOG_Error ("invalid stream parameter");
  }


  /* open hardware */
  if (vo_dope_initialized == 0)
  {
    /* init gui_state */
    memset (&gui_state, 0, sizeof (gui_state_t));

    /* init DOpE library */
    if (dope_init () != 0)
    {
      LOG_Error ("dope_init() failed.");
      return -L4_EOPEN;
    }
    /* register DOpE-application */
    gui_state.app_id = dope_init_app ("VSync");
    /* create dope event handler */
    l4thread_create ((void *) dope_event_handler_thread, NULL,
		     L4THREAD_CREATE_ASYNC);
    /* get screen size for fullscreen support */
    dope_req (gui_state.app_id, req_buf, 16, "screen.w");
    gui_state.scr_width = atoi (req_buf);
    dope_req (gui_state.app_id, req_buf, 16, "screen.h");
    gui_state.scr_height = atoi (req_buf);
    LOGdL (DEBUG_EXPORT, "screen size is %ix%i px.\n", gui_state.scr_width,
	   gui_state.scr_height);
    if ((gui_state.scr_height <= 0) || (gui_state.scr_width <= 0))
    {
      /* if we get wrong screen size we take 800x600 as default */
      LOG_Error ("screen size is 0x0! This can't be!");
      gui_state.scr_width = 800;
      gui_state.scr_height = 600;
    }
  }
  vo_dope_initialized = 1;

  /* init some gui_state vars */
  gui_state.pause = gui_state.fullscreen = 0;
  gui_state.osd_text = NULL;

  /* open window with rt-widget */
  dope_cmd (gui_state.app_id, "vernerwin=new Window()");
  dope_cmd (gui_state.app_id, "vernervscr=new VScreen()");

  /* set vscreen mode */
  dope_cmdf (gui_state.app_id, DOPE_CP_INIT_STR, info->vi.xdim,
	     info->vi.ydim);

  /* we use it asynchron. our metronom does this job
     but only 25 fps gives us a RT-Widget ! */
  dope_cmd (gui_state.app_id, "vernervscr.set(-framerate 25)");
  dope_cmd (gui_state.app_id, "vernerwin.set(-title \"Player\")");

  /* set window probs and center on screen */
  dope_cmdf (gui_state.app_id,
	     "vernerwin.set(-workx %d -worky %d -workw %d -workh %d -background off -content vernervscr)",
	     (gui_state.scr_width / 2) - (info->vi.xdim / 2),
	     (gui_state.scr_height / 2) - (info->vi.ydim / 2), info->vi.xdim,
	     info->vi.ydim);

  /* remember frame sizes */
  gui_state.xdim = info->vi.xdim;
  gui_state.ydim = info->vi.ydim;

  /* get identifier of pSLIM-server */
  gui_state.vernervscr_id =
    vscr_get_server_id (gui_state.app_id, "vernervscr");

  /* map vscreen buffer to local address space */
  gui_state.scr_addr = vscr_get_fb (gui_state.app_id, "vernervscr");
  if (!gui_state.scr_addr)
  {
    LOG_Error ("invalid address");
    /* close already opened windows */
    dope_cmd (gui_state.app_id, "vernervscr.set(-framerate 0)");

    /* close vscreen and window */
    dope_cmd (gui_state.app_id, "vernerwin.close()");
    return -L4_ENOMEM;
  }

  /* bind key events */
  dope_bind (gui_state.app_id, "vernervscr", "press", vo_dope_event_callback,
	     (void *) 0);

  /* calculate Framesize to copy to DOpE */
  info->vi.colorspace = DOPE_COLORSPACE;
  gui_state.memcpy_size =
    vid_streaminfo2packetsize (info) - sizeof (frame_ctrl_t);

  /* checking cpu caps for faster memcpy */
  fastmemcpy_func = determineFastMemcpy (attr->cpucaps);

  /* now show it */
  dope_cmd (gui_state.app_id, "vernerwin.open()");

  /* done */
  return 0;
}



/*
 * close dope export plugin
 */
int
vo_dope_close (plugin_ctrl_t * attr)
{
  /* nf2-hint: removes it at least from DOpEs RT-Manager */
  dope_cmd (gui_state.app_id, "vernervscr.set(-framerate 0)");

  /* close vscreen and window */
  dope_cmd (gui_state.app_id, "vernerwin.close()");

  /* free osd text */
  if (gui_state.osd_text)
    free (gui_state.osd_text);

  /* done */
  return 0;
}



/*
 * display frame and OSD
 */
int
vo_dope_step (plugin_ctrl_t * attr, void *addr)
{

  frame_ctrl_t *frameattr = (frame_ctrl_t *) addr;
  stream_info_t info;		/* for reconfigure */
  unsigned long start, end;
  int ret;

  if (attr->mode != PLUG_MODE_EXPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (!addr)
  {
    LOG_Error ("no data to export");
    return -L4_ENODATA;
  }

  /* get start time to measure time spend in this function */
  start = get_time_microsec ();
  
  /* reset attr means video resolution has changed within stream */
  if (is_reconfigure_point (frameattr->keyframe))
  {
    LOGdL (DEBUG_EXPORT, "reconfigure point detected - reinit screen.");
    memcpy (&info, frameattr, sizeof (frame_ctrl_t));
    /* close vscreen */
    vo_dope_close (attr);
    /* reopen new vscreen */
    if ((ret = vo_dope_init (attr, &info)) != 0)
    {
      LOG_Error ("Could not re-init dope (%d).", ret);
      return ret;
    }
  }				/* end reset attr */

#if RT_SYNC_WITH_DOPE
  /* to avoid flickering, we sync with the vscreen */
  vscr_server_waitsync (gui_state.vernervscr_id);
#endif
  /* let's show it :)
     We assume that we have the wanted format, so the size is constant */
  /* copy frame to DOpE vscreen shmem */
  (void) fastmemcpy_func (gui_state.scr_addr, addr + sizeof (frame_ctrl_t),
			  (size_t) gui_state.memcpy_size);

  /* 
   * simple OSD
   */

  /* got text to display? */
  if (strlen (attr->info))
  {
    if (gui_state.osd_text)
      free (gui_state.osd_text);
    gui_state.osd_text = strdup (attr->info);
    gui_state.osd_frames_to_display = 75;	/* display 3sec by 25fsp */
    /* ack info */
    attr->info[0] = '\0';
  }

  /* display */
  if ((gui_state.osd_text) && (gui_state.osd_frames_to_display))
  {
    yuv_image_printf ((unsigned char *) gui_state.scr_addr,
		      frameattr->vi.xdim, frameattr->vi.ydim, 10, 10,
		      gui_state.osd_text);
    gui_state.osd_frames_to_display--;
  }

  /* get end of waiting time */
  end = get_time_microsec ();
  attr->step_time_us = end - start;

  /* done */
  return 0;
}



/*
 * commit frame (unused)
 */
int
vo_dope_commit (plugin_ctrl_t * attr)
{
  /* done */
  return 0;
}
