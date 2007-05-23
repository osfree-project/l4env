/*
 * \brief   Overlay WM for Nitpicker
 * \date    2005-01-19
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004-2005  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4 INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/thread/thread.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/nitpicker/nitpicker-client.h>
#include <l4/nitpicker/nitevent-server.h>
#include <l4/nitpicker/event.h>
#include <l4/overlay_wm/overlay-server.h>
#include <l4/overlay_wm/input_listener-client.h>

char LOG_tag[9] = "nitovl";
l4_ssize_t l4libc_heapsize = 500*1024;


static CORBA_Object_base nit;              /* nitpicker server             */
static CORBA_Object_base nit_ev;           /* local event listener thread  */
static CORBA_Object_base client_listener;  /* event listener at the client */

static int scr_width, scr_height;          /* overlay screen dimensions    */
static int scr_mode;                       /* color depth                  */
static l4dm_dataspace_t scr_ds;            /* dataspace for overlay screen */
static void *scr_adr;                      /* local base address of screen */
static int scr_buf_id;                     /* nitpicker buffer id          */
static char *overlay_name = "OvlWM";       /* name of this server          */

struct window;
struct window {
	int x, y, w, h;                  /* current position and size          */
	int nit_id;                      /* id of corresponding nitpicker view */
	int ovl_id;                      /* local id                           */
	struct window *next;
};

struct window *first_win;
struct window *bg_win;


/*************************
 *** UTILITY FUNCTIONS ***
 *************************/

/*** FIND WINDOW STRUCT FOR A GIVEN OVL ID ***/
static struct window *find_window(int ovl_id) {
	struct window *curr = first_win;

	/* search for window with matching ovl id */
	while (curr && (curr->ovl_id != ovl_id))
		curr = curr->next;

	/* if such a window exists, return its struct */
	return curr;
}


/************************************
 *** IDL INTERFACE IMPLEMENTATION ***
 ************************************/

/*** IDL INTERFACE: REQUEST INFORMATION ABOUT PHYSICAL SCREEN ***/
int overlay_get_screen_info_component(CORBA_Object _dice_corba_obj,
                                      int *w, int *h, int *mode,
                                      CORBA_Server_Environment *_dice_corba_env) {
	printf("overlay_get_screen_info_component called wh=%d,%d\n", scr_width, scr_height);
	*w = scr_width;
	*h = scr_height;
	*mode = scr_mode;
	return 0;
}


/*** IDL INTERFACE: OPEN OVERLAY SCREEN WITH THE SPECIFIED PROPERTIES ***
 *
 * We should check, if the depth corresponds to Nitpicker's
 * color depth. We allocate a buffer of the requested size
 * and import the buffer into Nitpicker. NitOvlWM only uses
 * one Nitpicker buffer.
 */
int
overlay_open_screen_component(CORBA_Object _dice_corba_obj,
                              int width,
                              int height,
                              int depth,
                              CORBA_Server_Environment *_dice_corba_env) {
	int ret;
	CORBA_Environment env = dice_default_environment;
	printf("nitovlwm: open_screen called w=%d, h=%d, depth=%d\n", width, height, depth);

	/* open screen only once */
	if (scr_adr) return 0;

	scr_adr = l4dm_mem_ds_allocate(width*height*depth/8,
	                               L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED | L4RM_MAP,
	                               &scr_ds);
	if (!scr_adr) {
		printf("open_screen: l4dm_mem_ds_allocate returned %p\n", scr_adr);
		return -1;
	}

	/* enable nitpicker to access the buffer */
	ret = l4dm_share(&scr_ds, nit, L4DM_RW);

	scr_buf_id = nitpicker_import_buffer_call(&nit, &scr_ds, width, height, &env);
	printf("open_screen: nitpicker_import_buffer_call returned buf_id=%d\n", scr_buf_id);

	return 0;
}


/*** IDL INTERFACE: CLOSE OVERLAY SCREEN ***
 *
 * When the overlay screen is closed, we need to get rid of all
 * views that use the screen their buffer.
 */
void
overlay_close_screen_component(CORBA_Object _dice_corba_obj,
                               CORBA_Server_Environment *_dice_corba_env) {
	CORBA_Environment env = dice_default_environment;

	/* destroy all remaining overlay windows */
	while (first_win)
		overlay_destroy_window_component(NULL, first_win->ovl_id, NULL);

	if (scr_adr) nitpicker_remove_buffer_call(&nit, scr_buf_id, &env);

	/* free dataspace */
	l4dm_mem_release(scr_adr);
	scr_adr = NULL;
}


/*** IDL INTERFACE: MAP OVERLAY SCREEN BUFFER TO THE CLIENT ***/
int
overlay_map_screen_component(CORBA_Object _dice_corba_obj,
                             char* *ds_ident,
                             CORBA_Server_Environment *_dice_corba_env) {
	static char retbuf[128];
	int ret;

	/* grant access to the screen buffer to the client */
	ret = l4dm_share(&scr_ds, *_dice_corba_obj, L4DM_RW);
	printf("map_screen: l4dm_share to client returned %d\n", ret);

	/* convert dataspace id to ds_ident outstring */
	*ds_ident = retbuf;
	sprintf(retbuf, "t_id=0x%08lX,%08lX ds_id=0x%08x size=0x%08x",
	        scr_ds.manager.lh.low,
	        scr_ds.manager.lh.high,
	        scr_ds.id,
	        scr_width*scr_height*scr_mode/8);

	printf("nitovlwm: map_screen returns %s\n", retbuf);

	return 0;
}


/*** IDL INTERFACE: UPDATE REGION OF OVERLAY SCREEN AND AFFECTED WINDOWS ***/
void
overlay_refresh_screen_component(CORBA_Object _dice_corba_obj,
                                 int x,
                                 int y,
                                 int w,
                                 int h,
                                 CORBA_Server_Environment *_dice_corba_env) {
	CORBA_Environment env = dice_default_environment;

	nitpicker_refresh_call(&nit, scr_buf_id, x, y, w, h, &env);
}


/*** IDL INTERFACE: REGISTER LISTENER THREAD FOR INPUT EVENTS ***/
void
overlay_input_listener_component(CORBA_Object _dice_corba_obj,
                                 const_CORBA_Object listener,
                                 CORBA_Server_Environment *_dice_corba_env) {
	client_listener = *listener;
}


/*** IDL INTERFACE: REGISTER LISTENER THREAD FOR WINDOW EVENTS ***
 *
 * Nitpicker does not generate window events.
 */
void
overlay_window_listener_component(CORBA_Object _dice_corba_obj,
                                  const_CORBA_Object window_listener,
                                  CORBA_Server_Environment *_dice_corba_env) {
}


/*** IDL INTERFACE: CREATE NEW OVERLAY WINDOW ***/
int
overlay_create_window_component(CORBA_Object _dice_corba_obj,
                                CORBA_Server_Environment *_dice_corba_env) {
	static int ovl_id_cnt;

	struct window *new = malloc(sizeof(struct window));

	if (!new) {
		printf("Error: out of memory in function create_window_component\n");
		return -1;
	}

	new->next = first_win;
	first_win = new;

	new->ovl_id  = ++ovl_id_cnt;   /* assign unique ovl window id             */
	new->nit_id  = -1;             /* a newly created window has no view, yet */

	return new->ovl_id;
}


/*** IDL INTERFACE: DESTROY AN OVERLAY WINDOW ***/
void
overlay_destroy_window_component(CORBA_Object _dice_corba_obj,
                                 int win_id,
                                 CORBA_Server_Environment *_dice_corba_env) {
	struct window *win, *curr = first_win;

	/* try to close the window */
	overlay_close_window_component(NULL, win_id, NULL);

	if ((win = find_window(win_id)) == NULL) return;

	/* if window is the first in the list, skip it */
	if (win == first_win) first_win = win->next;
	else {

		/* search predecessor of window in list */
		while (curr && curr->next != win) curr = curr->next;

		/* skip window in list */
		if (curr) curr->next = win->next;
	}

	if (bg_win == win)
		bg_win = 0;

	free(win);
}


/*** IDL INTERFACE: MAKE OVERLAY WINDOW VISIBLE ON SCREEN ***/
void
overlay_open_window_component(CORBA_Object _dice_corba_obj,
                              int win_id,
                              CORBA_Server_Environment *_dice_corba_env) {
	CORBA_Environment env = dice_default_environment;
	struct window *win = find_window(win_id);

	if (!win) return;

	win->nit_id = nitpicker_new_view_call(&nit, scr_buf_id, &nit_ev, &env);

	if (win == bg_win) {
		printf("ok, we have a background view\n");
		nitpicker_set_background_call(&nit, win->nit_id, &env);
	}

	nitpicker_set_view_title_call(&nit, win->nit_id, "NitOvlWM", &env);
	nitpicker_set_view_port_call(&nit, win->nit_id, win->x, win->y,
	                             win->x, win->y, win->w, win->h, 1, &env);
}


/*** IDL INTERFACE: CLOSE OVERLAY WINDOW ***/
void
overlay_close_window_component(CORBA_Object _dice_corba_obj,
                               int win_id,
                               CORBA_Server_Environment *_dice_corba_env) {
	CORBA_Environment env = dice_default_environment;
	struct window *win = find_window(win_id);

	if (!win) return;
	nitpicker_destroy_view_call(&nit, win->nit_id, &env);
	win->nit_id = -1;
}


#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/*** IDL INTERFACE: SET POSITION AND SIZE OF AN OVERLAY WINDOW ***/
void
overlay_place_window_component(CORBA_Object _dice_corba_obj,
                               int win_id, int x, int y, int w, int h,
                               CORBA_Server_Environment *_dice_corba_env) {
	CORBA_Environment env = dice_default_environment;
	int ox1, oy1, ox2, oy2;
	int nx1, ny1, nx2, ny2;
	int min_y1, max_y1;
	int min_y2, max_y2;
	int min_x1, max_x1;
	int min_x2, max_x2;
	struct window *win = find_window(win_id);

	if (!win) return;

	ox1 = win->x; ox2 = win->x + win->w - 1;
	oy1 = win->y; oy2 = win->y + win->h - 1;

	win->x = x; win->y = y; win->w = w; win->h = h;
	if (win->nit_id >= 0)
		nitpicker_set_view_port_call(&nit, win->nit_id, x, y,
		                             x, y, w, h, 0, &env);

	nx1 = win->x; nx2 = win->x + win->w - 1;
	ny1 = win->y; ny2 = win->y + win->h - 1;

	/*
	 * Determine regions that are newly visible inside the view.
	 * These regions must be refreshed separately because we do
	 * not enable the refresh when setting the view port.
	 * This avoids double refresh (one refresh is coming in
	 * from the ovlscreen driver anyway).
	 */

	min_y1 = MIN(ny1, oy1);
	max_y1 = MAX(ny1, oy1);
	min_y2 = MIN(ny2, oy2);
	max_y2 = MAX(ny2, oy2);
	min_x1 = MIN(nx1, ox1);
	max_x1 = MAX(nx1, ox1);
	min_x2 = MIN(nx2, ox2);
	max_x2 = MAX(nx2, ox2);

	/* top */
	if (ny1 != oy1) {
		CORBA_exception_free(&env);
		nitpicker_refresh_call(&nit, scr_buf_id, min_x1, min_y1,
		                       max_x2 - min_x1 + 1, max_y1 - min_y1, &env);
	}

	/* bottom */
	if (ny2 != oy2) {
		CORBA_exception_free(&env);
		nitpicker_refresh_call(&nit, scr_buf_id, min_x1, min_y2 + 1,
		                       max_x2 - min_x1 + 1, max_y2 - min_y2, &env);
	}

	/* left */
	if (nx1 != ox1) {
		CORBA_exception_free(&env);
		nitpicker_refresh_call(&nit, scr_buf_id, min_x1, max_y1,
		                       max_x1 - min_x1, min_y2 - max_y1 + 1, &env);
	}

	/* right */
	if (nx2 != ox2) {
		CORBA_exception_free(&env);
		nitpicker_refresh_call(&nit, scr_buf_id, min_x2 + 1, max_y1,
		                       max_x2 - min_x2, min_y2 - max_y1 + 1, &env);
	}
}


/*** IDL INTERFACE: BRING OVERLAY WINDOW ON TOP ***/
void
overlay_stack_window_component(CORBA_Object _dice_corba_obj,
                               int win_id,
                               int neighbor_id,
                               int behind,
                               int do_redraw,
                               CORBA_Server_Environment *_dice_corba_env) {

	CORBA_Environment env   = dice_default_environment;
	struct window *win      = find_window(win_id);
	struct window *neighbor = find_window(neighbor_id);

	int view_id          = win      ? win->nit_id      : -1;
	int neighbor_view_id = neighbor ? neighbor->nit_id : -1;

	if (view_id < 0) return;
	nitpicker_stack_view_call(&nit, view_id, neighbor_view_id,
	                          behind, do_redraw, &env);
}


/*** IDL INTERFACE: DEFINE TITLE OF AN OVERLAY WINDOW ***/
void overlay_title_window_component(CORBA_Object _dice_corba_obj,
                                    int win_id, const char* title,
                                    CORBA_Server_Environment *_dice_corba_env) {

	CORBA_Environment env = dice_default_environment;
	struct window *win = find_window(win_id);
	if (!win) return;
	nitpicker_set_view_title_call(&nit, win->nit_id, title, &env);
}


/*** IDL INTERFACE: DEFINE BACKGROUND WINDOW ***/
void overlay_set_background_component(CORBA_Object _dice_corba_obj, int win_id,
                                      CORBA_Server_Environment *_dice_corba_env) {

	CORBA_Environment env = dice_default_environment;
	struct window *win = find_window(win_id);
	if (!win) return;

	bg_win = win;

	nitpicker_set_background_call(&nit, win->nit_id, &env);
}


/*************************************
 *** PROCESS EVENTS FROM NITPICKER ***
 *************************************/

void
nitevent_event_component(CORBA_Object _dice_corba_obj,
                         unsigned long token,
                         int type,
                         int keycode,
                         int rx,
                         int ry,
                         int ax,
                         int ay,
                         CORBA_Server_Environment *_dice_corba_env) {
	CORBA_Environment env = dice_default_environment;
	static int old_ax, old_ay;

	/* deliver motion event if absolute pointer position changed */
	CORBA_exception_free(&env);
	if ((old_ax != ax) || (old_ay != ay)) {
		input_listener_motion_call(&client_listener, ax, ay, rx, ry, &env);
		if (DICE_HAS_EXCEPTION(&env))
			printf("nitevent_event_component: IPC error %d for event forwarding motion call\n", DICE_IPC_ERROR(&env));
		else {
			old_ax = ax;
			old_ay = ay;
		}
	}

	CORBA_exception_free(&env);
	switch (type) {
		case NITEVENT_TYPE_PRESS:
			input_listener_button_call(&client_listener, 1, keycode, &env);
			if (DICE_HAS_EXCEPTION(&env))
				printf("nitevent_event_component: IPC error %d for event forwarding press call\n", DICE_IPC_ERROR(&env));
			break;

		case NITEVENT_TYPE_RELEASE:
			input_listener_button_call(&client_listener, 2, keycode, &env);
			if (DICE_HAS_EXCEPTION(&env))
				printf("nitevent_event_component: IPC error %d for event forwarding release call\n", DICE_IPC_ERROR(&env));
			break;
	}
}


/*** MAIN PROGRAM ***/
int main(int argc, char **argv) {
	int i, ret;
	void *buf_adr;
	l4thread_t event_thread;
	l4dm_dataspace_t ds;
	CORBA_Environment env = dice_default_environment;

	/* set identifier of overlay server if specified as argument */
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "--name") && (i + 1 < argc))
			overlay_name = argv[i + 1];

	if (names_waitfor_name("Nitpicker", &nit, 10000) == 0) {
		printf("Nitpicker is not registered at names!\n");
		return 1;
	}

	/* donate memory to nitpicker */
	buf_adr = l4dm_mem_ds_allocate(100*1024,
	                               L4DM_CONTIGUOUS | L4RM_LOG2_ALIGNED | L4RM_MAP,
	                               &ds);
	printf("l4dm_mem_ds_allocate returned %p\n", buf_adr);

	ret = l4dm_share(&ds, nit, L4DM_RW);
	printf("l4dm_share returned %d\n", ret);

	CORBA_exception_free(&env);
	ret = nitpicker_donate_memory_call(&nit, &ds, 100, 1, &env);
	printf("nitpicker_donate_memory_call returned %d\n", ret);

	CORBA_exception_free(&env);
	nitpicker_get_screen_info_call(&nit, &scr_width, &scr_height, &scr_mode, &env);

	/* start event thread that listens to input events from nitpicker */
	printf("nitovlwm: start event thread\n");
	event_thread = l4thread_create_named(nitevent_server_loop,
	                                     "nitevent", NULL,
	                                     L4THREAD_CREATE_ASYNC);
	nit_ev = l4thread_l4_id(event_thread);

	/* register overlay wm server and start serving */
	if (!names_register(overlay_name)) {
		printf("Error: could not register as \"%s\" at names\n", overlay_name);
		return -1;
	}
	printf("nitovlwm: start server loop\n");
	overlay_server_loop(NULL);
	return 0;
}
