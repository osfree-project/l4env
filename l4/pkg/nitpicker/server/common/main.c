/*
 * \brief   Nitpicker main program
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** L4 INCLUDES ***/
#include <l4/input/macros.h>
#include <l4/nitpicker/event.h>

/* FIXME: DICE should name the error functions for client and server different */
#include <l4/nitpicker/nitevent-client.h>   /* deliver input events */

/*** LOCAL INCLUDES ***/
#include "nitpicker.h"

#include "bigmouse.c"             /* graphics for the mouse cursor */

static char    self_client_struct[sizeof(client) + 10*sizeof(view) + 10*sizeof(buffer)];
static char    menu_text[64];
static int     menu_buf_id;
static buffer *menu_buf;
static int     mouse_view_id;                /* view id of mouse cursor */
static int     mouse_w, mouse_h;             /* size of mouse cursor    */
int            mode      = 0;                /* current nitpicker mode  */
static int     userstate = USERSTATE_IDLE;   /* current user state      */
int            mouse_x, mouse_y;             /* current mouse position  */
extern view   *curr_view;                    /* currently focused view  */
CORBA_Object   myself;

static CORBA_Environment env = dice_default_environment;


/*** INTERFACE: PROVIDE INFORMATION ABOUT THE PHYSICAL SCREEN ***/
int nitpicker_get_screen_info_component(CORBA_Object _dice_corba_obj,
                                    int *w, int *h, int *mode,
                                    CORBA_Server_Environment *_dice_corba_env) {
	*w    = scr_width;
	*h    = scr_height;
	*mode = scr_depth;
	return 0;
}


/*** SET TITLE IN MENUBAR ***/
void menubar_set_text(char *text) {
	void *dst = menu_buf->data;
	int i;

	/* draw menu background */
	for (i = 0; i < 16; i++) {
		u32 color = GFX_RGB(100 - i*4, 100 - i*4, (!mode ? 100 : 150)  - i*4);
		gfx->draw_box(dst, 0, i, scr_width, 1, color);
	}

	/* set new menu text */
	if (text) strncpy(menu_text, text, sizeof(menu_text));

	/* draw string */
	if (!push_clipping(0, 0, scr_width, 16)) {
		int txpos = (scr_width - font_string_width(default_font, menu_text)) / 2;
		DRAW_LABEL(dst, txpos, 2, GFX_RGB(0, 0, 0), GFX_RGB(255, 255, 255), menu_text);
		pop_clipping();
	}
	nitpicker_refresh_component(myself, menu_buf_id, 0, 0, scr_width, 16, 0);
}


/*** REFRESH THE WHOLE SCREEN ***
 *
 * This function should is called on the mode change.
 */
static void refresh_screen(void) {
	menubar_set_text(NULL);
	draw_rec(first_view, NULL, NULL, 0, 0, scr_width - 1, scr_height - 1);
}


/****************************
 *** INPUT EVENT HANDLING ***
 ****************************/

/*** SET MOUSE CURSOR TO THE SPECIFIED POSITION ***/
static void set_mousepos(int mx, int my) {

	if ((mx == mouse_x) && (my == mouse_y)) return;

	/* clip mouse position against screen area */
	if (mx < 0) mx = 0;
	if (my < 0) my = 0;
	if (mx > scr_width  - 1) mx = scr_width  - 1;
	if (my > scr_height - 1) my = scr_height - 1;

	mouse_x = mx;
	mouse_y = my;

	/* tell the nitpicker main thread to reposition the mouse view */
	if (!nit_server) return;
	nitpicker_set_view_port_component(nit_server, mouse_view_id, 0, 0,
	                                  mx, my, mouse_w, mouse_h, 1, 0);
}


static inline void count_keys(int type) {
	static int num_keys;

	if (type == NITEVENT_TYPE_PRESS)   num_keys++;
	if (type == NITEVENT_TYPE_RELEASE) num_keys--;
	if (num_keys <  0) num_keys  = 0;
	if (num_keys == 0) userstate = USERSTATE_IDLE;
}


/*** HANDLE USER INPUT WHEN KILL MODE IS ACTIVE ***/
static void handle_kill_input(int type, int code, int rx, int ry) {

	count_keys(type);

	/* update mouse pointer */
	if (type == NITEVENT_TYPE_MOTION)
		set_mousepos(mouse_x + rx, mouse_y + ry);

	/* update userstate according to press/release event */
	else if (type == NITEVENT_TYPE_PRESS) {

		view *v = find_view(mouse_x, mouse_y);

		switch (code) {

			case BTN_LEFT:

				/* kill client but do not commit suicide */
				if (v && !dice_obj_eq(&v->owner, myself))
					remove_client(&v->owner);

				/* fall through */

			case KEY_PRINT:

				/* leave kill mode */
				mode &= ~MODE_KILL;
				refresh_screen();
		}
	}
}


/*** HANDLE USER INPUT IN NORMAL MODE ***/
void handle_normal_input(int type, int code, int rx, int ry) {
	view *v = curr_view;

	count_keys(type);

	if (type == NITEVENT_TYPE_MOTION) {

		/* update mouse cursor position (assign new values to mouse_x and mouse_y) */
		set_mousepos(mouse_x + rx, mouse_y + ry);

		/* determine affected view */
		v = (userstate == USERSTATE_IDLE) ? find_view(mouse_x, mouse_y) : curr_view;

		/* deliver one motion event */
		nitevent_event_send(&v->listener, v->token, NITEVENT_TYPE_MOTION, 0,
		                    rx, ry, mouse_x, mouse_y, &env);
		return;
	}

	/* update userstate according to press/release event */
	if (type == NITEVENT_TYPE_PRESS) {

		/* a magic key let us toggle between secure and normal mode */
		if (code == KEY_SCROLLLOCK) {

			/* toggle secure and flat mode */
			mode = (mode & MODE_SECURE) ? (mode & ~MODE_SECURE) : (mode | MODE_SECURE);
			refresh_screen();

		} else if (code == KEY_PRINT) {

			/* tint screen in red -> kill mode */
			mode |= MODE_KILL;
			refresh_screen();

			/* block and handle kill mode */
			while (mode & MODE_KILL)
				foreach_input_event(handle_kill_input);

		} else if (code == BTN_LEFT) {

			userstate = USERSTATE_DRAG;
			activate_view(find_view(mouse_x, mouse_y));
			v = curr_view;
		}
	}

	/*
	 * Deliver press/release event to the client application but
	 * prevent magic keystrokes to be reported to the client.
	 */
	if (v && (code != KEY_SCROLLLOCK) && (code != KEY_PRINT))
		nitevent_event_send(&v->listener, v->token,
	                        type, code, 0, 0, mouse_x, mouse_y, &env);
}


/*** DEFAULT INITIALIZATION FOR NITPICKERS OWN VIEWS ***/
static void setup_view(int view_id, int add_flags, int remove_flags, int bg) {

	view *v = lookup_view(myself, view_id);
	if (!v) return;

	v->flags       = (v->flags | add_flags) & ~remove_flags;
	v->label[0]    = 0;
	v->client_name = "";
	if (bg) v->buf = NULL;
}


/*** NITPICKER MAIN PROGRAM ***/
int main(int argc, char **argv) {
	static int bg_view_id, menu_view_id, mouse_buf_id;
	buffer *b;

	env.timeout = L4_IPC_TIMEOUT(0, 1, 128, 11, 0, 0);

	printf("sizeof(client) = %d\nsizeof(view)   = %d\nsizeof(buffer) = %d\n",
	       sizeof(client), sizeof(view), sizeof(buffer));

	CORBA_Object_base dummy = l4_myself();
	myself = &dummy;

	/* init and install memory space for our own client */
	add_client(myself, &self_client_struct[0], 10, 10);

	/* sub-system initialization */
	TRY(native_startup(argc, argv), "Native startup failed");
	TRY(scr_init(),   "Screen init failed");
	TRY(input_init(), "Input init failed");

	/* create buffer for the mouse cursor graphics */
	mouse_buf_id = nitpicker_import_buffer_component(myself, 0, 16, 16, 0);
	if ((b = lookup_buffer(myself, mouse_buf_id))) {
		b->w    =  bigmouse_trp.width;
		b->h    =  bigmouse_trp.height;
		b->data = &bigmouse_trp.pixels[0][0];
	}

	/* create view for the mouse cursor */
	mouse_view_id = nitpicker_new_view_component(myself, mouse_buf_id, 0, 0);
	mouse_w = mouse_h = 16;

	/* configure mouse view */
	setup_view(mouse_view_id,
	           VIEW_FLAGS_STAYTOP | VIEW_FLAGS_TRANSPARENT | VIEW_FLAGS_FRONT,
	           VIEW_FLAGS_BORDER, 0);

	/* define initial mouse cursor position */
	set_mousepos(0, 0);

	font_init();
	clipping_init(0, 0, scr_width, scr_height);

	/* create desktop background view */
	bg_view_id = nitpicker_new_view_component(myself, 0, 0, 0);
	setup_view(bg_view_id, VIEW_FLAGS_BACKGROUND, 0, 1);
	nitpicker_set_view_port_component(myself, bg_view_id, 0, 0, 0, 0, scr_width, scr_height, 1, 0);

	/* create menubar view */
	menu_buf_id = nitpicker_import_buffer_component(myself, 0, scr_width, 16, 0);
	menu_buf = lookup_buffer(myself, menu_buf_id);
	menu_buf->w = scr_width;
	menu_buf->h = 16;
	menu_buf->data = malloc(scr_width*2*16);
	menubar_set_text("");

	menu_view_id = nitpicker_new_view_component(myself, menu_buf_id, 0, 0);
	setup_view(menu_view_id, VIEW_FLAGS_STAYTOP | VIEW_FLAGS_FRONT, 0, 0);
	nitpicker_set_view_port_component(myself, menu_view_id, 0, 0, 0, 0, scr_width, 16, 1, 0);

	start_server();   /* this function does not return */
	return 0;
}
